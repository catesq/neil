#pragma once

#include "aeffectx.h"
#include "zzub/plugin.h"




struct vst_parameter {
    vst_parameter(VstParameterProperties*, zzub::parameter* zzub_param, uint16_t index);

    virtual float zzub_to_vst_value(uint16_t zzub) = 0;
    virtual uint16_t vst_to_zzub_value(float vst) = 0;

    static vst_parameter* build(VstParameterProperties* vst_props, zzub::parameter* zzub_param, uint16_t index);

    VstParameterProperties* vst_props;
    zzub::parameter* zzub_param;
    uint16_t index;
};


struct vst_switch_parameter : vst_parameter {
    using vst_parameter::vst_parameter;

    virtual float zzub_to_vst_value(uint16_t) override;
    virtual uint16_t vst_to_zzub_value(float vst) override;
};


struct vst_int_parameter : vst_parameter {
    vst_int_parameter(VstParameterProperties*, zzub::parameter* zzub_param, uint16_t index);

    virtual float zzub_to_vst_value(uint16_t) override;
    virtual uint16_t vst_to_zzub_value(float vst) override;

    int min, max, vst_range;
};


struct vst_float_parameter : vst_parameter {
    vst_float_parameter(VstParameterProperties*, zzub::parameter* zzub_param, uint16_t index);

    virtual float zzub_to_vst_value(uint16_t value) override;
    virtual uint16_t vst_to_zzub_value(float vst) override;

    bool useStep;
    int min, max;
    float smallStep, largeStep;
    int range;
};


// struct vst_port : zzub::port {
//     vst_port();
// };


// struct vst_audio_port : vst_port {
//     vst_audio_port();

// };

// struct vst_cv_port : vst_port {

// };


// struct vst_parameter_port :vst_port {

// };

// struct vst_midi_port : vst_port {

// };