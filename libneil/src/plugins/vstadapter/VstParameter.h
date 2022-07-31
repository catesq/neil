#pragma once

#include "zzub/plugin.h"
#include "aeffectx.h"


struct VstParameter {
    VstParameter(VstParameterProperties*, const zzub::parameter* zzub_param, uint16_t offset);

    virtual float zzub_to_vst_value(uint16_t zzub) { return 0.f; }
    virtual uint16_t vst_to_zzub_value(float vst)  { return 0; }

    static VstParameter* build(VstParameterProperties* vst_props, const zzub::parameter* zzub_param, uint16_t offset);

    VstParameterProperties* vst_props;
    const zzub::parameter* zzub_param;

    // Byte size of this data in global_values, either 2 or 1 as zzub currently only handle data either 8 or 16 bit int
    uint16_t data_size;

    // The offset of the data in global_value
    uint16_t data_offset;
};

struct VstSwitchParameter : VstParameter {
    using VstParameter::VstParameter;

    virtual float zzub_to_vst_value(uint16_t);
    virtual uint16_t vst_to_zzub_value(float vst);
};

struct VstIntParameter : VstParameter {
    using VstParameter::VstParameter;

    virtual float zzub_to_vst_value(uint16_t );
    virtual uint16_t vst_to_zzub_value(float vst);
};

struct VstFloatParameter : VstParameter {
    using VstParameter::VstParameter;

    virtual float zzub_to_vst_value(uint16_t value);
    virtual uint16_t vst_to_zzub_value(float vst);
};
