#pragma once

#include "zzub/plugin.h"
#include "aeffectx.h"

union zzub_value {
    uint8_t byte;
    uint16_t word;
    zzub_value(uint8_t b) : byte(b) {}
    zzub_value(uint16_t w) : word(w) {}
};

struct VstParameter {
    VstParameter(VstParameterProperties*, zzub::parameter* zzub_param);

    virtual float zzub_to_vst_value(zzub_value zzub) = 0;
    virtual zzub_value vst_to_zzub_value(float vst) = 0;

    static VstParameter* build(VstParameterProperties* vst_props, zzub::parameter* zzub_param);
protected:
    VstParameterProperties* vst_props;
    zzub::parameter* zzub_param;
};

struct VstSwitchParameter : VstParameter {
    using VstParameter::VstParameter;

    virtual float zzub_to_vst_value(zzub_value zzub);
    virtual zzub_value vst_to_zzub_value(float vst);
//    VstSwitchParameter(VstParameterProperties* vst_props, zzub::parameter* zzub_param);
protected:
};

struct VstIntParameter : VstParameter {
    using VstParameter::VstParameter;

    virtual float zzub_to_vst_value(zzub_value value);
    virtual zzub_value vst_to_zzub_value(float vst);
//    VstIntParameter(VstParameterProperties* vst_props, zzub::parameter* zzub_param);
protected:
};

struct VstFloatParameter : VstParameter {
    using VstParameter::VstParameter;

    virtual float zzub_to_vst_value(zzub_value value);
    virtual zzub_value vst_to_zzub_value(float vst);
//    VstFloatParameter(VstParameterProperties* vst_props, zzub::parameter* zzub_param);
protected:

};
