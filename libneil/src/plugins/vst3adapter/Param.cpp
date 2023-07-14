
#include "Param.h"


Vst3Param* 
Vst3Param::build(Steinberg::Vst::ParameterInfo& param_info) {
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


float Vst3ToggleParam::to_vst_value(uint32_t zzub_val) const {
    if(zzub_val == zzub::switch_value_on)
        return 1.0f;
    else
        return 0.0f;
}


uint32_t Vst3ToggleParam::to_zzub_value(double zzub_val) const {
    if(zzub_val > 0.5f)
        return zzub::switch_value_on;
    else
        return zzub::switch_value_off;
}


float Vst3FloatParam::to_vst_value(uint32_t zzub_val) const {
    return (float)zzub_val / 32768.0f;
}


uint32_t Vst3FloatParam::to_zzub_value(double zzub_val) const {
    return (uint32_t)(zzub_val * 32768.0f);
}


float Vst3IntParam::to_vst_value(uint32_t zzub_val) const {
    return (float)zzub_val / (float)zzub_param.value_max;
}


uint32_t Vst3IntParam::to_zzub_value(double zzub_val) const {
    return (uint32_t)(zzub_val * (float)zzub_param.value_max);
}