#pragma once

namespace zzub {


struct sequence_pattern_event {
    int value;
    int length;
};

struct sequence_wave_event {
    int wave;
    int offset;
    int length;
};

struct sequence_automation_event {
    int value;
};

struct sequence_event {
    int time;
    union {
        sequence_pattern_event pattern_event;
        sequence_wave_event wave_event;
        sequence_automation_event automation_event;
    };
};


}