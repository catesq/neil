#pragma once

#include "zzub/plugin.h"
#include "public.sdk/source/vst/hosting/plugprovider.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"

#include "Defines.h"
#include "Param.h"


enum class Vst3Category {
    Unknown, Instrument, Effect, Controller
};

struct Vst3Info : zzub::info {
    Vst3Info(
        std::string filename,
        const VST3::Hosting::PluginFactory& factory,
        const VST3::Hosting::ClassInfo& class_info
    );

    virtual ~Vst3Info();

    virtual zzub::plugin* create_plugin() const;

    virtual bool store_info(zzub::archive *) const { return false; }

    std::string get_filename() const;
    
    Vst3Category get_main_category(const VST3::Hosting::ClassInfo&) const;

    bool is_valid() const;

private:
    std::string filename;
    const VST3::Hosting::PluginFactory& factory;
    const VST3::Hosting::ClassInfo& class_info;

    std::vector<Vst3Param*> params;
    bool is_valid_plugin = false;
};

