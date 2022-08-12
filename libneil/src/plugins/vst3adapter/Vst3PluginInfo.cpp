#include "Vst3PluginInfo.h"


#include <string>
#include <iostream>
#include "public.sdk/source/vst/utility/stringconvert.h"


struct Vst3Param {
    Vst3Param(Steinberg::Vst::ParameterInfo& param_info)
        : id(param_info.id),
          name(VST3::StringConvert::convert(param_info.title)),
          short_name(VST3::StringConvert::convert(param_info.title)),
          units(VST3::StringConvert::convert(param_info.units)),
          defaultValue(param_info.defaultNormalizedValue) {
    }

    uint32_t id;
    std::string name;
    std::string short_name;
    std::string units;
    double defaultValue;

    static Vst3Param* build(Steinberg::Vst::ParameterInfo& param_info);
};

struct Vst3FloatParam : Vst3Param {
    using Vst3Param::Vst3Param;
};

struct Vst3ToggleParam : Vst3Param {
    using Vst3Param::Vst3Param;
};

struct Vst3IntParam : Vst3Param {
    Vst3IntParam(Steinberg::Vst::ParameterInfo& param_info)
        : Vst3Param(param_info),
          step_count(param_info.stepCount) {

    }

    uint32_t step_count;
};

Vst3Param* Vst3Param::build(Steinberg::Vst::ParameterInfo& param_info) {
    switch(param_info.stepCount) {
    case 0:
        return new Vst3FloatParam(param_info);
        break;
    case 1:
        return new Vst3ToggleParam(param_info);
        break;
    default:
        return new Vst3IntParam(param_info);
    }
}



Vst3PluginInfo::Vst3PluginInfo(std::string filename, VST3::Hosting::ClassInfo* class_info, Steinberg::Vst::PlugProvider* provider)
    : zzub::info(),
      filename(filename) {
    auto plugin = provider->getComponent();
    auto controller = provider->getController();
//    auto factory = provider->getPluginFactory();

    for(int idx=0; idx < controller->getParameterCount(); idx++) {
        Steinberg::Vst::ParameterInfo param_info{};
        auto result = controller->getParameterInfo(idx, param_info);
        if(result != Steinberg::kResultOk)
            continue;

        params.push_back(Vst3Param::build(param_info));
    }

    printf("vst name: %s\n", class_info->name().c_str());
    author = class_info->vendor().c_str();
    name = class_info->name().c_str();
    short_name = class_info->name().c_str();
    version = (int) (std::stod(class_info->version()) * 1000);
    uri = "@zzub.org/vst3adapter/" + name + "/" + std::to_string(version);

    switch(get_main_category(class_info)) {
        Effect:
            flags |= (zzub::plugin_flag_is_effect | zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output | zzub::plugin_flag_is_effect);
            break;
        Instrument:
            min_tracks = 1;
            max_tracks = 16;
            flags |= zzub::plugin_flag_has_midi_input;
            add_track_parameter().set_note();
            add_track_parameter().set_byte()
                                 .set_name("Volume")
                                 .set_description("Volume")
                                 .set_value_min(0)
                                 .set_value_max(0x007F)
                                 .set_value_none(VOLUME_NONE)
                                 .set_value_default(VOLUME_DEFAULT);

            flags |= (zzub::plugin_flag_is_instrument | zzub::plugin_flag_has_audio_output | zzub::plugin_flag_is_instrument);
            break;

        Controller:
            flags |= zzub::plugin_flag_control_plugin;

        default:
            break;
    }

    provider->releasePlugIn(plugin, controller);
    controller->release();
    plugin->release();
}

Vst3Category Vst3PluginInfo::get_main_category(VST3::Hosting::ClassInfo* class_info) const {
    bool is_instrument;

    for(auto& category: class_info->subCategories()) {
        if(category == "Instrument" || category == "Synth")
            return Vst3Category::Instrument;
        else if (category == "Fx")
            return Vst3Category::Effect;
    }

    printf("Category not recognised for plugin %s. Categories were: ", class_info->name().c_str());
    for(auto& category: class_info->subCategories())
        printf("%s ", category.c_str());
    printf("\n");

    return Vst3Category::Unknown;
}

/// TODO make a move constructor and a move constructor - to reuse/free the malloc'd strings in the zuub::param in global_parameters and reallocte
/// the plugininfo will be reused and only be destroyed when the program is closed so putting this off is unclean but irrelevant
Vst3PluginInfo::~Vst3PluginInfo() {
    for(auto param: params)
        free(param);

    params.clear();
}



zzub::plugin* Vst3PluginInfo::create_plugin() const {
    return nullptr;
}

