#pragma once

#include "zzub/plugin.h"
#include "aeffectx.h"


struct VstParameter {
    VstParameter(VstParameterProperties*, zzub::parameter* zzub_param, uint16_t index);

    virtual float zzub_to_vst_value(uint16_t zzub) = 0;
    virtual uint16_t vst_to_zzub_value(float vst)  = 0;

    static VstParameter* build(VstParameterProperties* vst_props, zzub::parameter* zzub_param, uint16_t index);

    VstParameterProperties* vst_props;
    zzub::parameter* zzub_param;
    uint16_t index;
};

struct VstSwitchParameter : VstParameter {
    using VstParameter::VstParameter;

    virtual float zzub_to_vst_value(uint16_t) override;
    virtual uint16_t vst_to_zzub_value(float vst) override;
};

struct VstIntParameter : VstParameter {
    VstIntParameter(VstParameterProperties*, zzub::parameter* zzub_param, uint16_t index);

    virtual float zzub_to_vst_value(uint16_t ) override;
    virtual uint16_t vst_to_zzub_value(float vst) override;

    int min, max, vst_range;
};

struct VstFloatParameter : VstParameter {
    VstFloatParameter(VstParameterProperties*, zzub::parameter* zzub_param, uint16_t index);

    virtual float zzub_to_vst_value(uint16_t value) override;
    virtual uint16_t vst_to_zzub_value(float vst) override;

    bool useStep;
    int min, max;
    float smallStep, largeStep;
    int range;
};
