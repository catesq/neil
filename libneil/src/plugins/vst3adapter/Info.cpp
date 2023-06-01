#include "Info.h"


#include <string>
#include <iostream>
#include "public.sdk/source/vst/utility/stringconvert.h"


Vst3Info::Vst3Info(
    std::string filename,
    const VST3::Hosting::PluginFactory& factory,
    const VST3::Hosting::ClassInfo& class_info
) : zzub::info(),
    filename(filename),
    factory(factory),
    class_info(class_info) {

    auto* provider = new Steinberg::Vst::PlugProvider(factory, class_info, true);
    auto controller = provider->getController();
    auto plugin_component = provider->getComponent();
    auto audio_effect = Steinberg::FUnknownPtr<Steinberg::Vst::IAudioProcessor>(plugin_component);

    if (audio_effect) {
        is_valid_plugin = true;
    } else {
        std::cerr << "Error getting audio processor VST3 plugin \"" << filename << std::endl;
        return;
    }

    for(int idx=0; idx < controller->getParameterCount(); idx++) {
        Steinberg::Vst::ParameterInfo param_info {};

        auto result = controller->getParameterInfo(idx, param_info);

        if(result != Steinberg::kResultOk)
            continue;

        params.push_back(Vst3Param::build(param_info));
    }

    author = class_info.vendor().c_str();
    name = class_info.name().c_str();
    short_name = class_info.name().c_str();
    version = (int) (std::stod(class_info.version()) * 1000);
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

    provider->releasePlugIn(plugin_component, controller);
    controller->release();

    // why does this crash?
    // provider->release();
}


Vst3Category Vst3Info::get_main_category(const VST3::Hosting::ClassInfo& class_info) const {
    bool is_instrument;

    for(auto& category: class_info.subCategories()) {
        if(category == "Instrument" || category == "Synth")
            return Vst3Category::Instrument;
        else if (category == "Fx")
            return Vst3Category::Effect;
    }

    printf("Category not recognised for plugin %s. Categories were: ", class_info.name().c_str());
    for(auto& category: class_info.subCategories())
        printf("%s ", category.c_str());
    printf("\n");

    return Vst3Category::Unknown;
}


/// TODO make a move constructor and a move constructor - to reuse/free the malloc'd strings in the zuub::param in global_parameters and reallocte
/// the plugininfo will be reused and only be destroyed when the program is closed so putting this off is unclean but irrelevant
Vst3Info::~Vst3Info() {
    for(auto param: params)
        free(param);

    params.clear();
}


bool Vst3Info::is_valid() const {
    return is_valid_plugin;
}


zzub::plugin* Vst3Info::create_plugin() const {
    return nullptr;
}

