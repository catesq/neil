#pragma once
#include <zzub/plugin.h>


struct DrumParameter : zzub::parameter {
    float mapMin, mapMax, mapMul;
    int paramOffs=0;
    const char *unit = "";
    std::vector<std::string> options{};

    DrumParameter(const char *name, int offs, float min, float max, float d_fault, const char *unit);
    DrumParameter(const char *name, int offs, bool switch_default);
    DrumParameter(const char *name, int offs, int num, int d_fault, const char* unit);
    DrumParameter(const char *name, int offs, int num, int d_fault, std::vector<std::string> options);

    const char *describe_value(int value) const;
};
