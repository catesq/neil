#pragma once

#include "aeffect.h"
#include "aeffectx.h"
#include "vst_parameter.h"
#include "zzub/plugin.h"

struct vst_zzub_info : zzub::info {
    vst_zzub_info(AEffect* plugin, std::string filename, VstPlugCategory category);
    virtual ~vst_zzub_info();

    virtual zzub::plugin* create_plugin() const;

    virtual bool store_info(zzub::archive*) const { return false; }

    std::string get_filename() const;
    int get_param_count() const;
    bool get_is_synth() const;
    const std::vector<std::string>& get_param_names() const;
    vst_parameter* get_vst_param(int index) const;
    const std::vector<vst_parameter*>& get_vst_params() const;

   private:
    int32_t vst_id;
    std::string filename;
    VstPlugCategory category;
    std::vector<std::string> param_names;
    std::vector<vst_parameter*> vst_params;
};
