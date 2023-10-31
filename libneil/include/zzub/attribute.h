#pragma once

namespace zzub {

struct attribute {
    const char *name;
    int value_min;
    int value_max;
    int value_default;

    attribute() {
        name = "";
        value_min = 0;
        value_max = 0;
        value_default = 0;
    }

    attribute &set_name(const char *name) {
        this->name = name;
        return *this;
    }
    attribute &set_value_min(int value_min) {
        this->value_min = value_min;
        return *this;
    }
    attribute &set_value_max(int value_max) {
        this->value_max = value_max;
        return *this;
    }
    attribute &set_value_default(int value_default) {
        this->value_default = value_default;
        return *this;
    }
};

}