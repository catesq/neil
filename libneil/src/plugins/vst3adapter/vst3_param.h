#pragma once

#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "public.sdk/source/vst/utility/stringconvert.h"

#include "zzub/plugin.h"


struct Vst3Param {
    uint32_t param_id;

    std::string name;

    std::string short_name;

    std::string units;

    double defaultValue;

    zzub::parameter zzub_param {};

    Vst3Param(Steinberg::Vst::ParameterInfo& param_info)
        : param_id(param_info.id),
          name(VST3::StringConvert::convert(param_info.title)),
          short_name(VST3::StringConvert::convert(param_info.title)),
          units(VST3::StringConvert::convert(param_info.units)),
          defaultValue(param_info.defaultNormalizedValue) {
            zzub_param.name = name.c_str();
            zzub_param.description = name.c_str();
            zzub_param.flags = zzub::parameter_flag_state;
    }

    virtual float to_vst_value(uint32_t zzub_val) const = 0;
    
    virtual uint32_t to_zzub_value(double vst_val) const = 0;

    static Vst3Param* build(Steinberg::Vst::ParameterInfo& param_info);
};


struct Vst3FloatParam : Vst3Param {
    // using Vst3Param::Vst3Param;

     Vst3FloatParam(Steinberg::Vst::ParameterInfo& param_info)
        : Vst3Param(param_info) {
            zzub_param.type = zzub::parameter_type_word;
            zzub_param.value_min = 0;
            zzub_param.value_max = 32768;
            zzub_param.value_none = 65535;
            zzub_param.value_default = to_zzub_value(param_info.defaultNormalizedValue);
        }

    virtual float to_vst_value(uint32_t zzub_val) const override;
    virtual uint32_t to_zzub_value(double vst_val) const override;
};  


struct Vst3ToggleParam : Vst3Param {
     Vst3ToggleParam(Steinberg::Vst::ParameterInfo& param_info)
        : Vst3Param(param_info) {
            zzub_param.type = zzub::parameter_type_word;
            zzub_param.value_min = zzub::switch_value_off;
            zzub_param.value_max = zzub::switch_value_on;
            zzub_param.value_none = zzub::switch_value_none;
            zzub_param.value_default = to_zzub_value(param_info.defaultNormalizedValue);
        }

    virtual float to_vst_value(uint32_t zzub_val) const override;
    virtual uint32_t to_zzub_value(double vst_val) const override;
};


struct Vst3IntParam : Vst3Param {
    Vst3IntParam(Steinberg::Vst::ParameterInfo& param_info)
        : Vst3Param(param_info),
          step_count(param_info.stepCount) {
            zzub_param.type = zzub::parameter_type_word;
            zzub_param.value_min = 0;
            zzub_param.value_max = std::max(param_info.stepCount, 65534);
            zzub_param.value_none = 65535;
            zzub_param.value_default = to_zzub_value(param_info.defaultNormalizedValue);
    }

    virtual float to_vst_value(uint32_t zzub_val) const override;
    virtual uint32_t to_zzub_value(double vst_val) const override;

    uint32_t step_count;
};


