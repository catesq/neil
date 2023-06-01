
#include "Param.h"


Vst3Param* Vst3Param::build(Steinberg::Vst::ParameterInfo& param_info) {
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


