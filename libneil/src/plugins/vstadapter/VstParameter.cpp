#include "VstParameter.h"
#include <algorithm>

VstParameter* VstParameter::build(VstParameterProperties* properties, const zzub::parameter* zzub_param, uint16_t offset) {
    if(!properties)
        return new VstParameter(properties, zzub_param, offset);
    else if(properties->flags & kVstParameterIsSwitch)
        return new VstSwitchParameter(properties, zzub_param, offset);
    else if (properties->flags & kVstParameterUsesIntegerMinMax)
        return new VstIntParameter(properties, zzub_param, offset);
    else
        return new VstFloatParameter(properties, zzub_param, offset);
}


VstParameter::VstParameter(VstParameterProperties* vst_props, const zzub::parameter* zzub_param, uint16_t offset) : vst_props(vst_props), zzub_param(zzub_param), data_size(2), data_offset(offset) { }



float VstSwitchParameter::zzub_to_vst_value(uint16_t zzub_val) {
    return zzub_val;
}

uint16_t VstSwitchParameter::vst_to_zzub_value(float vst) {
    return vst > 0.5f ? 1 : 0;
}




float VstIntParameter::zzub_to_vst_value(uint16_t zzub_val) {
    return vst_props->minInteger +
        ((zzub_val - zzub_param->value_min) / (float) (zzub_param->value_max - zzub_param->value_min)) * (vst_props->maxInteger - vst_props->minInteger);
}

uint16_t VstIntParameter::vst_to_zzub_value(float vst_val) {
    return zzub_param->value_min + (int)(((vst_val - vst_props->minInteger) / (vst_props->maxInteger - vst_props->minInteger)) * (zzub_param->value_max - zzub_param->value_min));
}



float VstFloatParameter::zzub_to_vst_value(uint16_t zzub_val) {
    return std::clamp(((float) (zzub_val) / zzub_param->value_max), 0.f, 1.0f);
}

uint16_t VstFloatParameter::vst_to_zzub_value(float vst) {
    return std::clamp(vst, 0.0f, 1.0f) * zzub_param->value_max;
}
