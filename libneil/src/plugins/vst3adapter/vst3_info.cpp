#include <string>
#include <iostream>

#include "public.sdk/source/vst/utility/stringconvert.h"
#include "pluginterfaces/gui/iplugview.h"

#include "vst3_info.h"
#include "vst3_adapter.h"

#include <algorithm>

using MediaTypes = Steinberg::Vst::MediaTypes;
using BusDirections = Steinberg::Vst::BusDirections;



Vst3Info::Vst3Info(
    std::string filename,
    VST3::Hosting::Module::Ptr host_module,
    VST3::Hosting::ClassInfo class_info,
    Steinberg::Vst::PlugProvider* provider
) : zzub::info(),
    filename(filename),
    host_module(host_module),
    class_info(class_info)
{
    auto controller = provider->getController();
    auto plugin_component = provider->getComponent();
    auto audio_processor = Steinberg::FUnknownPtr<Steinberg::Vst::IAudioProcessor>(plugin_component);

    if (audio_processor) {
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

    author = class_info.vendor();
    name = class_info.name  ();
    short_name = class_info.name();
    version = (int) (std::stod(class_info.version()) * 1000);
    uri = "@zzub.org/vst3adapter/" + name + "/" + std::to_string(version);

    switch(get_main_category(class_info)) {
        case Vst3Category::Effect:
            flags |= (zzub::plugin_flag_is_effect | zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output);
            break;

        case Vst3Category::Instrument:
            zzub::midi_track_manager::add_midi_track_info(this);
            flags |= (zzub::plugin_flag_is_instrument | zzub::plugin_flag_has_audio_output | zzub::plugin_flag_has_midi_input);
            break;

        case Vst3Category::Controller:
            flags |= zzub::plugin_flag_control_plugin;
            break;

        case Vst3Category::Unknown:
            printf("plugin %s category is unknown\n", name.c_str());
            break;
    }


    flags |= zzub::plugin_flag_has_custom_gui;  


    Steinberg::FUnknownPtr<Steinberg::Vst::IProcessContextRequirements> contextRequirements(plugin_component);
    
    if(contextRequirements) {
        printf("contextRequirements for %s: %p\n", name.c_str(), contextRequirements);
        requirement_flags = contextRequirements->getProcessContextRequirements();
    } else {
        printf("no      contextRequirements for %s\n", name.c_str());
    }

    bus_infos.audio_in  = build_bus_infos(plugin_component, MediaTypes::kAudio, BusDirections::kInput);
    bus_infos.audio_out = build_bus_infos(plugin_component, MediaTypes::kAudio, BusDirections::kOutput);
    bus_infos.event_in  = build_bus_infos(plugin_component, MediaTypes::kEvent, BusDirections::kInput);
    bus_infos.event_out = build_bus_infos(plugin_component, MediaTypes::kEvent, BusDirections::kOutput);

    int parameter_count = controller->getParameterCount();
    for(int index=0; index < parameter_count; index++) {
        Steinberg::Vst::ParameterInfo param_info;
        controller->getParameterInfo(index, param_info);

        Vst3Param* param = Vst3Param::build(param_info);

        if(parameter_count < 20)
            printf(
                "Plugin name: %s, Param name: %s, step count: %d\n", 
                name.c_str(), VST3::StringConvert::convert(param_info.title).c_str(), param_info.stepCount
            );

        params.push_back(param);
        global_parameters.push_back(&param->zzub_param);
    }

    provider->releasePlugIn(plugin_component, controller);
    // why does this crash?
    // provider->release();
}



/// TODO make a move constructor and a move constructor - to reuse/free the malloc'd strings in the zuub::param in global_parameters and reallocte
/// the plugininfo will be reused and only be destroyed when the program is closed so putting this off is unclean but irrelevant
Vst3Info::~Vst3Info() 
{
    for(auto param: params)
        free(param);

    params.clear();
}



const std::vector<Steinberg::Vst::BusInfo>& Vst3Info::get_bus_infos
(
    Steinberg::Vst::MediaTypes type,
    Steinberg::Vst::BusDirections direction
) const {
    static auto empty_infos = std::vector<Steinberg::Vst::BusInfo>();

    switch(type) {
        case MediaTypes::kAudio:
            if(direction == BusDirections::kInput)
                return bus_infos.audio_in;
            else
                return bus_infos.audio_out;

        case MediaTypes::kEvent:
            if(direction == BusDirections::kInput)
                return bus_infos.event_in;
            else
                return bus_infos.event_out;

        default:
            return empty_infos;
    }
}



const Steinberg::Vst::BusInfo& Vst3Info::get_bus_info
(
    Steinberg::Vst::MediaTypes type,
    Steinberg::Vst::BusDirections direction,
    uint32_t index
) const {
    return get_bus_infos(type, direction)[index];
}



uint32_t Vst3Info::get_bus_count
(
    Steinberg::Vst::MediaTypes media_type,
    Steinberg::Vst::BusDirections direction
) const {
    return get_bus_infos(media_type, direction).size();
}


std::vector<Steinberg::Vst::BusInfo> Vst3Info::build_bus_infos
(
    Steinberg::Vst::IComponent* plugin,
    Steinberg::Vst::MediaTypes type, 
    Steinberg::Vst::BusDirections direction
) {
    std::vector<Steinberg::Vst::BusInfo> bus_infos;

    for(int index=0; index < plugin->getBusCount(type, direction); index++)  {
        Steinberg::Vst::BusInfo bus_info;
        plugin->getBusInfo(type, direction, index, bus_info);
        bus_infos.push_back(bus_info);
    }

    return bus_infos;
}



uint32_t 
Vst3Info::get_global_param_count() const 
{
    return global_parameters.size();
}



uint32_t 
Vst3Info::get_track_param_count() const 
{
    return track_parameters.size();
}



Vst3Category 
Vst3Info::get_main_category
(
    const VST3::Hosting::ClassInfo& class_info
) const {
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



Vst3Param* 
Vst3Info::get_vst_param(uint32_t index) const 
{
    return params[index];
}



bool 
Vst3Info::is_valid() const 
{
    return is_valid_plugin;
}



zzub::plugin* 
Vst3Info::create_plugin() const 
{
    auto factory = host_module->getFactory();
    auto provider = new Steinberg::Vst::PlugProvider(factory, class_info, true);
    
    return new Vst3PluginAdapter(this, provider);
}

