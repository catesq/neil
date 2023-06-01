#pragma once

#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "public.sdk/source/vst/utility/stringconvert.h"


struct Vst3Param {
    uint32_t id;

    std::string name;

    std::string short_name;

    std::string units;

    double defaultValue;

    Vst3Param(Steinberg::Vst::ParameterInfo& param_info)
        : id(param_info.id),
          name(VST3::StringConvert::convert(param_info.title)),
          short_name(VST3::StringConvert::convert(param_info.title)),
          units(VST3::StringConvert::convert(param_info.units)),
          defaultValue(param_info.defaultNormalizedValue) {
    }

    static Vst3Param* build(Steinberg::Vst::ParameterInfo& param_info);
};


struct Vst3FloatParam : Vst3Param {
    using Vst3Param::Vst3Param;
};


struct Vst3ToggleParam : Vst3Param {
    using Vst3Param::Vst3Param;
};


struct Vst3IntParam : Vst3Param {
    Vst3IntParam(Steinberg::Vst::ParameterInfo& param_info)
        : Vst3Param(param_info),
          step_count(param_info.stepCount) {
    }

    uint32_t step_count;
};


