#include "VstParameter.h"
#include <algorithm>


vst_parameter* vst_parameter::build(VstParameterProperties* properties, zzub::parameter* zzub_param, uint16_t index) {
    if(!properties)
        return new vst_float_parameter(properties, zzub_param, index);
    else if(properties->flags & kVstParameterIsSwitch)
        return new vst_switch_parameter(properties, zzub_param, index);
    else if (properties->flags & kVstParameterUsesIntegerMinMax && !(properties->flags & kVstParameterUsesFloatStep))
        return new vst_int_parameter(properties, zzub_param, index);
    else
        return new vst_float_parameter(properties, zzub_param, index);
}



vst_parameter::vst_parameter(VstParameterProperties* vst_props,
                           zzub::parameter* zzub_param,
                           uint16_t index)
    : vst_props(vst_props),
      zzub_param(zzub_param),
      index(index)
{

}



float 
vst_switch_parameter::zzub_to_vst_value(uint16_t zzub_val) 
{
    return zzub_val;
}



uint16_t 
vst_switch_parameter::vst_to_zzub_value(float vst) 
{
    return vst > 0.5f ? 1 : 0;
}



VstIntParameter::VstIntParameter(VstParameterProperties* vst_props,
                                 zzub::parameter* zzub_param,
                                 uint16_t index) : vst_parameter(vst_props, zzub_param, index)
{
    min = vst_props->minInteger;
    max = vst_props->maxInteger;

    vst_range = vst_props->maxInteger - vst_props->minInteger;
//    printf("vst int: min %d, max %d, %s\n\n", zzub_param->value_min, zzub_param->value_max, zzub_param->name);

    if (vst_range <= 32768){
        zzub_param->value_max = vst_range;
    }
}



float 
vst_int_parameter::zzub_to_vst_value(uint16_t zzub_val) 
{
    return zzub_val / (float) vst_range;
}



uint16_t 
vst_int_parameter::vst_to_zzub_value(float vst_val) 
{
    //    printf("int param. min %d max %d vst range %d vst_val %f\n", min, max, vst_range, vst_val);
    return vst_val * vst_range;
}



VstFloatParameter::VstFloatParameter(VstParameterProperties* vst_props,
                                     zzub::parameter* zzub_param,
                                     uint16_t index) 
    : vst_parameter(vst_props, zzub_param, index),
      min(0),
      max(1)
{
    // float is the default parameter type so use some sane defaults if vst properties not supplied
    if(!vst_props) {
        range = 1;
        max = 1;
        min = 0;
        return;
    }

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



float 
VstFloatParameter::zzub_to_vst_value(uint16_t zzub_val) 
{
    float prop = ( zzub_val / (float) zzub_param->value_max);
//    printf("float zzub to vst val: zzub_val %d, vst_val %.3f", zzub_val, std::clamp(prop * range - min, (float) min, (float) max));
    return std::clamp(prop * range - min, (float) min, (float) max);
}



uint16_t 
VstFloatParameter::vst_to_zzub_value(float vst_val) 
{
    float prop = (vst_val - min) / range;
    uint16_t val = std::clamp(prop, 0.0f, 1.0f) * zzub_param->value_max;
//    printf("float vst to zuub val: vst_val %f, vst_min %d, zzub_max %d, zzub_val %d, name %s\n", vst_val, min, zzub_param->value_max, val, zzub_param->name);
    return std::clamp(prop, 0.0f, 1.0f) * zzub_param->value_max;
}
