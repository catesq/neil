#pragma once

#include "Vst3Defines.h"
#include "zzub/plugin.h"
#include "public.sdk/source/vst/hosting/plugprovider.h"

struct Vst3Param;

enum class Vst3Category {
    Unknown, Instrument, Effect, Controller
};

struct Vst3PluginInfo : zzub::info {
    Vst3PluginInfo(std::string filename, VST3::Hosting::ClassInfo*, Steinberg::Vst::PlugProvider*);
    virtual ~Vst3PluginInfo();

    virtual zzub::plugin* create_plugin() const;
    virtual bool store_info(zzub::archive *) const { return false; }

    std::string get_filename() const;
    Vst3Category get_main_category(VST3::Hosting::ClassInfo*) const;

private:
    std::vector<Vst3Param*> params;
    std::string filename;
};

