#pragma once

#include "zzub/consts.h"

namespace zzub {


struct parameter {
    parameter_type type;
    const char *name;
    const char *description;
    int value_min;
    int value_max;
    int value_none;
    int flags;
    int value_default;

    parameter() {
        type = parameter_type_switch;
        name = 0;
        description = 0;
        value_min = 0;
        value_max = 0;
        value_none = 0;
        flags = 0;
        value_default = 0;
    }

    parameter &set_type(parameter_type type) {
        this->type = type;
        return *this;
    }

    parameter &set_note() {
        this->type = parameter_type_note;
        this->name = "Note";
        this->description = "Note";
        this->value_min = note_value_min;
        this->value_max = note_value_max;
        this->value_none = note_value_none;
        this->value_default = this->value_none;
        return *this;
    }
    parameter &set_switch() {
        this->type = parameter_type_switch;
        this->name = "Switch";
        this->description = "Switch";
        this->value_min = switch_value_off;
        this->value_max = switch_value_on;
        this->value_none = switch_value_none;
        this->value_default = this->value_none;
        return *this;
    }
    parameter &set_byte() {
        this->type = parameter_type_byte;
        this->name = "Byte";
        this->description = "Byte";
        this->value_min = 0;
        this->value_max = 128;
        this->value_none = 255;
        this->value_default = this->value_none;
        return *this;
    }
    parameter &set_word() {
        this->type = parameter_type_word;
        this->name = "Word";
        this->description = "Word";
        this->value_min = 0;
        this->value_max = 32768;
        this->value_none = 65535;
        this->value_default = this->value_none;
        return *this;
    }
    parameter &set_wavetable_index() {
        this->type = parameter_type_byte;
        this->name = "Wave";
        this->description = "Wave to use (01-C8)";
        this->value_min = wavetable_index_value_min;
        this->value_max = wavetable_index_value_max;
        this->value_none = wavetable_index_value_none;
        this->flags = parameter_flag_wavetable_index;
        this->value_default = 0;
        return *this;
    }
    parameter &set_name(const char *name) {
        this->name = name;
        return *this;
    }
    parameter &set_description(const char *description) {
        this->description = description;
        return *this;
    }
    parameter &set_value_min(int value_min) {
        this->value_min = value_min;
        return *this;
    }
    parameter &set_value_max(int value_max) {
        this->value_max = value_max;
        return *this;
    }
    parameter &set_value_none(int value_none) {
        this->value_none = value_none;
        return *this;
    }
    parameter &set_flags(int flags) {
        this->flags = flags;
        return *this;
    }
    parameter &set_state_flag() {
        this->flags |= zzub::parameter_flag_state;
        return *this;
    }
    parameter &set_wavetable_index_flag() {
        this->flags |= zzub::parameter_flag_wavetable_index;
        return *this;
    }
    parameter &set_event_on_edit_flag() {
        this->flags |= zzub::parameter_flag_event_on_edit;
        return *this;
    }
    parameter &set_value_default(int value_default) {
        this->value_default = value_default;
        return *this;
    }

    float normalize(int value) const {
        assert(value != this->value_none);
        return float(value - this->value_min) / float(this->value_max - this->value_min);
    }

    int scale(float normal) const {
        return int(normal * float(this->value_max - this->value_min) + 0.5) + this->value_min;
    }

    int get_bytesize() const {
        switch (this->type) {
            case parameter_type_note:
            case parameter_type_switch:
            case parameter_type_byte:
                return 1;
            case parameter_type_word:
                return 2;
            default:
                return 0;
        }
        return 0;
    }

    parameter &append(std::vector<const parameter *> &paramlist) {
        paramlist.push_back(this);
        return *this;
    }
};
}