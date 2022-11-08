#include "VstParameter.h"
#include <algorithm>


VstParameter* VstParameter::build(VstParameterProperties* properties, zzub::parameter* zzub_param, uint16_t offset) {
    if(!properties)
        return new VstFloatParameter(properties, zzub_param, offset);
    else if(properties->flags & kVstParameterIsSwitch)
        return new VstSwitchParameter(properties, zzub_param, offset);
    else if (properties->flags & kVstParameterUsesIntegerMinMax && !(properties->flags & kVstParameterUsesFloatStep))
        return new VstIntParameter(properties, zzub_param, offset);
    else
        return new VstFloatParameter(properties, zzub_param, offset);
}



VstParameter::VstParameter(VstParameterProperties* vst_props, zzub::parameter* zzub_param, uint16_t offset)
        : vst_props(vst_props),
          zzub_param(zzub_param),
          data_size(2),
          data_offset(offset) { }



float VstSwitchParameter::zzub_to_vst_value(uint16_t zzub_val) {
    return zzub_val;
}



uint16_t VstSwitchParameter::vst_to_zzub_value(float vst) {
    return vst > 0.5f ? 1 : 0;
}



VstIntParameter::VstIntParameter(VstParameterProperties* vst_props, zzub::parameter* zzub_param, uint16_t offset) :
        VstParameter(vst_props, zzub_param, offset) {

    if(!vst_props)
        return;

    min = vst_props->minInteger;
    max = vst_props->maxInteger;

    vst_range = vst_props->maxInteger - vst_props->minInteger;

    if (vst_range <= 32768){
        zzub_param->value_max = vst_range;
    }
}



float VstIntParameter::zzub_to_vst_value(uint16_t zzub_val) {
    return zzub_val / (float) vst_range;
}



uint16_t VstIntParameter::vst_to_zzub_value(float vst_val) {
//    printf("int param. min %d max %d vst range %d vst_val %f\n", min, max, vst_range, vst_val);
    return vst_val * vst_range;
}



VstFloatParameter::VstFloatParameter(VstParameterProperties* vst_props, zzub::parameter* zzub_param, uint16_t offset) :
        VstParameter(vst_props, zzub_param, offset),
        min(0),
        max(1) {
    if(!vst_props)
        return;

    if(vst_props->flags & kVstParameterUsesIntegerMinMax) {
        min = vst_props->minInteger;
        max = vst_props->maxInteger;
    }

    if(vst_props->flags & kVstParameterUsesFloatStep) {
        useStep = true;
        smallStep = vst_props->smallStepFloat;
        largeStep = vst_props->largeStepFloat;
    }

    range = max - min;
}



float VstFloatParameter::zzub_to_vst_value(uint16_t zzub_val) {
    float prop = ( zzub_val / (float) zzub_param->value_max);
    return std::clamp(prop * range - min, (float) min, (float) max);
}



uint16_t VstFloatParameter::vst_to_zzub_value(float vst_val) {
    float prop = (vst_val - min) / range;
    return std::clamp(prop, 0.0f, 1.0f) * zzub_param->value_max;
}
