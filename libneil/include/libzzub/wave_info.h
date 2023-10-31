#pragma once

#include <vector>
#include "zzub/consts.h"


namespace zzub {

struct envelope_info {
    const char *name;
    int flags;
};

struct envelope_point {
    unsigned short x, y;
    unsigned char flags;  // flags: bit 0 = sustain
};

struct envelope_entry {
    unsigned short attack, decay, sustain, release;
    char subDivide, flags;  // ADSR Subdivide & Flags
    bool disabled;
    std::vector<envelope_point> points;

    envelope_entry();
    void clear();
};

struct wave_info {
    int flags;
    float volume;
};

// the order of the members in wave_level are matched with buzz' CWaveInfo
// members prefixed with legacy_ should be handled in buzz2zzub and removed from here
struct wave_level {
    int legacy_sample_count;
    short *legacy_sample_ptr;
    int root_note;
    int samples_per_second;
    int legacy_loop_start;
    int legacy_loop_end;
    int sample_count;
    short *samples;
    int loop_start;
    int loop_end;
    int format;               // zzub_wave_buffer_type
    std::vector<int> slices;  // slice offsets in the wave

    int get_bytes_per_sample() const {
        switch (format) {
            case wave_buffer_type_si16:
                return 2;
            case wave_buffer_type_si24:
                return 3;
            case wave_buffer_type_si32:
                return 4;
            case wave_buffer_type_f32:
                return 4;
            default:
                assert(false);
                return 0;
        }
    }
};


}