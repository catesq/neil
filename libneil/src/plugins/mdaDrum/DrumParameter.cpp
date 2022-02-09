#include "DrumParameter.h"
#include <zzub/plugin.h>



DrumParameter::DrumParameter(const char *name, int offs, float min, float max, float d_fault, const char *unit) : zzub::parameter() {
    this->type = zzub::parameter_type_word;
    this->name = name;
    this->description = name;
    this->unit = unit;
    this->flags = zzub::parameter_flag_state;
    this->paramOffs = offs;

    this->mapMin = min;
    this->mapMax = mapMax;

    float map_diff = max - min;

    if(map_diff < 32) {
        this->mapMul = 1000.f;
    } else if (map_diff < 320) {
        this->mapMul = 100.f;
    } else if(map_diff < 3200){
        this->mapMul = 10.f;
    } else if(map_diff < 32000) {
        this->mapMul = 1.f;
    }

    this->value_min = 0;

    this->value_max = (int) (map_diff * mapMul);
    this->value_none = 65535;
    this->value_default = (int) (d_fault - mapMin) * mapMul;
}

DrumParameter::DrumParameter(const char *name, int offs, bool switch_default) : zzub::parameter() {
    this->mapMin = 0.f;
    this->mapMin = 1.f;
    this->mapMul = 1.f;

    this->type = zzub::parameter_type_word;
    this->name = name;
    this->description = name;
    this->paramOffs = offs;
    this->value_min = zzub::switch_value_off;
    this->value_max = zzub::switch_value_on;
    this->value_none = zzub::switch_value_none;
    this->value_default = switch_default ? zzub::switch_value_on : zzub::switch_value_off;
}

DrumParameter::DrumParameter(const char *name, int offs, int num, int d_fault, const char* unit) : zzub::parameter() {
    this->mapMin = 0.f;
    this->mapMin = (float) num;
    this->mapMul = 1.f;
    this->unit = unit;

    this->type = zzub::parameter_type_word;
    this->name = name;
    this->description = name;
    this->paramOffs = offs;
    this->value_min = 0;
    this->value_max = (float) num;
    this->value_none = 65535;
    this->value_default = (int) d_fault;
}

DrumParameter::DrumParameter(const char *name, int offs, int num, int d_fault, std::vector<std::string> options)
    : DrumParameter(name, offs, num, d_fault, "") {
    this->options = options;
}

const char *DrumParameter::describe_value(int value) const {
//        printf("options size %lu. value %d\n", options.size(), value);
    if(options.size()) {
        return value >=0 && value < options.size() ? options[value].c_str() : "describe_value: out of range";
    } if(mapMul == 1.f) {
        return (std::to_string(value) + unit).c_str();
    } else {
        return (std::to_string(((float) value) / mapMul) + unit).c_str();
    }
}








