#pragma once

#include "VstParameter.h"
#include "zzub/plugin.h"
#include "aeffect.h"
#include "aeffectx.h"


struct VstPluginInfo : zzub::info {
    VstPluginInfo(AEffect* plugin, std::string filename, VstPlugCategory category);
    virtual ~VstPluginInfo();

    virtual zzub::plugin* create_plugin() const;

    virtual bool store_info(zzub::archive *) const { return false; }

    std::string get_filename() const;
    int get_param_count() const;
    bool get_is_synth() const;
    const std::vector<std::string>& get_param_names() const;
    VstParameter* get_vst_param(int index) const;

private:
    int32_t vst_id;
    std::string filename;
    VstPlugCategory category;
    std::vector<std::string> param_names;
    std::vector<VstParameter*> vst_params;
};

