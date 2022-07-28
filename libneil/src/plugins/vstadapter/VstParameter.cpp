#include "VstParameter.h"
#include <algorithm>

VstParameter* VstParameter::build(VstParameterProperties* properties, zzub::parameter* zzub_param) {
    if(properties->flags & kVstParameterIsSwitch)
        return new VstSwitchParameter(properties, zzub_param);
    else if (properties->flags & kVstParameterUsesIntegerMinMax)
        return new VstIntParameter(properties, zzub_param);
    else
        return new VstFloatParameter(properties, zzub_param);
}


VstParameter::VstParameter(VstParameterProperties* vst_props, zzub::parameter* zzub_param) : vst_props(vst_props), zzub_param(zzub_param) { }



//VstSwitchParameter::VstSwitchParameter(VstParameterProperties* vst_props, zzub::parameter* zzub_param) : VstParameter(vst_props, zzub_param) {  }

float VstSwitchParameter::zzub_to_vst_value(zzub_value zzub) {
    return (float) zzub.word;
}

zzub_value VstSwitchParameter::vst_to_zzub_value(float vst) {
    return zzub_value(uint16_t(
        vst > 0.5f ? 1 : 0)
    );
}



//VstIntParameter::VstIntParameter(VstParameterProperties* vst_props, zzub::parameter* zzub_param) : VstParameter(vst_props, zzub_param) {  }


float VstIntParameter::zzub_to_vst_value(zzub_value zzub_val) {
    return vst_props->minInteger +
        ((zzub_val.word - zzub_param->value_min) / (float) (zzub_param->value_max - zzub_param->value_min)) * (vst_props->maxInteger - vst_props->minInteger);
}

zzub_value VstIntParameter::vst_to_zzub_value(float vst_val) {
    return zzub_value((uint16_t)(
        zzub_param->value_min + (int)(((vst_val - vst_props->minInteger) / (vst_props->maxInteger - vst_props->minInteger)) * (zzub_param->value_max - zzub_param->value_min))
    ));
}


//VstFloatParameter::VstFloatParameter(VstParameterProperties* vst_props, zzub::parameter* zzub_param) : VstParameter(vst_props, zzub_param) {  }


float VstFloatParameter::zzub_to_vst_value(zzub_value zzub) {
    return std::clamp(((float) (zzub.word) / zzub_param->value_max), 0.f, 1.0f);
}

zzub_value VstFloatParameter::vst_to_zzub_value(float vst) {
    return zzub_value(uint16_t(
        std::clamp(vst, 0.0f, 1.0f) * zzub_param->value_max
    ));
}
