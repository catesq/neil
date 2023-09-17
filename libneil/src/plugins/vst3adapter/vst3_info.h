#pragma once

#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <public.sdk/source/vst/hosting/plugprovider.h>

#include "Defines.h"
#include "Param.h"
#include "zzub/plugin.h"
 

enum class Vst3Category {
    Unknown,
    Instrument,
    Effect,
    Controller
};

struct Vst3Info : zzub::info {
    Vst3Info(std::string filename,
            VST3::Hosting::Module::Ptr host_module,
            VST3::Hosting::ClassInfo class_info,
            Steinberg::Vst::PlugProvider* provider);

    virtual ~Vst3Info();

    virtual zzub::plugin *create_plugin() const;

    virtual bool store_info(zzub::archive *) const { return false; }

    bool is_valid() const;

    std::string get_filename() const;

    uint32_t get_global_param_count() const;

    uint32_t get_track_param_count() const;

    Vst3Param* get_vst_param(uint32_t index) const;

    Vst3Category get_main_category(const VST3::Hosting::ClassInfo &) const;

    uint32_t get_bus_count(Steinberg::Vst::MediaTypes type, Steinberg::Vst::BusDirections direction) const;

    const std::vector<Steinberg::Vst::BusInfo>& get_bus_infos(Steinberg::Vst::MediaTypes type,
                                                       Steinberg::Vst::BusDirections direction) const;

    const Steinberg::Vst::BusInfo& get_bus_info(Steinberg::Vst::MediaTypes type,
                                                      Steinberg::Vst::BusDirections direction,
                                                      uint32_t index) const;

private:
    std::string filename;

    VST3::Hosting::ClassInfo class_info;
    VST3::Hosting::Module::Ptr host_module;

    std::vector<Vst3Param *> params;

    bool is_valid_plugin = false;
    uint32_t requirement_flags = 0;

    struct {
        std::vector<Steinberg::Vst::BusInfo> audio_in;
        std::vector<Steinberg::Vst::BusInfo> audio_out;
        std::vector<Steinberg::Vst::BusInfo> event_in;
        std::vector<Steinberg::Vst::BusInfo> event_out;
    } bus_infos = {{}, {}, {}, {}};

    std::vector<Steinberg::Vst::BusInfo> build_bus_infos(Steinberg::Vst::IComponent* plugin,
                                                         Steinberg::Vst::MediaTypes type,
                                                         Steinberg::Vst::BusDirections direction);
};