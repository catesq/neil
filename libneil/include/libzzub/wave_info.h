#pragma once

#include <vector>
#include "zzub/consts.h"
#include <cmath>
#include <cassert>


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


struct wave_level_ex : wave_level {
    wavelevel_proxy* proxy;

    wave_level_ex() {
        proxy = 0;
        samples = 0;
        sample_count = 0;
        loop_start = 0;
        loop_end = 0;
        samples_per_second = 0;
        legacy_sample_ptr = 0;
        legacy_sample_count = 0;
        legacy_loop_start = 0;
        legacy_loop_end = 0;
        format = wave_buffer_type_si16;
    }
};

struct wave_info {
    int flags;
    float volume;
};


struct wave_info_ex : wave_info {

    std::string fileName;
    std::string name;
    std::vector<envelope_entry> envelopes;
    std::vector<wave_level_ex> levels;
    wave_proxy* proxy;

    wave_info_ex();
    wave_info_ex(const wave_info_ex& w);
    ~wave_info_ex();

    int get_levels() const {
        return (int)levels.size();
    }

    wave_level_ex* get_level(int level) {
        if (level < 0 || (size_t)level >= levels.size()) return 0;
        return &levels[level];
    }

    bool get_extended() const {
        return flags&wave_flag_extended?true:false;
    }

    bool get_stereo() const {
        return flags&zzub::wave_flag_stereo?true:false;
    }

    void set_stereo(bool state) {
        unsigned f = ((unsigned)flags)&(0xFFFFFFFF^zzub::wave_flag_stereo);

        if (state)
            flags = f|zzub::wave_flag_stereo; else
            flags = f;
    }

    void* get_sample_ptr(int level, int offset=0) {
        wave_level* l = get_level(level);
        if (!l) return 0;
        offset *= get_bytes_per_sample(level) * (get_stereo()?2:1);
        if (get_extended()) {
            return (char*)&l->legacy_sample_ptr[4] + offset;
        } else
            return (char*)l->legacy_sample_ptr + offset;
    }

    int get_bits_per_sample(int level) {
        wave_level* l = get_level(level);
        if (!l) return 0;

        if (!get_extended()) return 16;

        switch (l->legacy_sample_ptr[0]) {
        case zzub::wave_buffer_type_si16:
            return 16;
        case zzub::wave_buffer_type_si24:
            return 24;
        case zzub::wave_buffer_type_f32:
        case zzub::wave_buffer_type_si32:
            return 32;
        default:
            //std::cerr << "Unknown extended sample format:" << l->samples[0] << std::endl;
            return 16;
        }
    }

    inline int get_bytes_per_sample(int level) {
        return get_bits_per_sample(level) / 8;
    }

    inline unsigned int get_extended_samples(int level, int samples) {
        int channels = get_stereo()?2:1;
        return ((samples-(4/channels)) *2 ) / get_bytes_per_sample(level);
    }

    inline unsigned int get_unextended_samples(int level, int samples) {
        int channels = get_stereo()?2:1;
        return (unsigned int)ceil((samples * get_bytes_per_sample(level)) / 2.0f) + (4/channels);
    }

    unsigned int get_sample_count(int level) {
        wave_level* l = get_level(level);
        if (!l) return 0;
        if (get_extended())
            return get_extended_samples(level, l->legacy_sample_count); else
            return l->legacy_sample_count;
    }

    unsigned int get_loop_start(int level) {
        wave_level* l = get_level(level);
        if (!l) return 0;
        if (get_extended())
            return get_extended_samples(level, l->legacy_loop_start); else
            return l->legacy_loop_start;
    }

    unsigned int get_loop_end(int level) {
        wave_level* l = get_level(level);
        if (!l) return 0;
        if (get_extended())
            return get_extended_samples(level, l->legacy_loop_end); else
            return l->legacy_loop_end;
    }

    void set_loop_start(int level, unsigned int value) {
        wave_level* l = get_level(level);
        if (!l) return ;
        if (get_extended()) {
            l->legacy_loop_start = get_unextended_samples(level, value);
        } else {
            l->legacy_loop_start = value;
        }
    }

    void set_loop_end(int level, int value) {
        wave_level* l = get_level(level);
        if (!l) return ;
        if (get_extended()) {
            l->legacy_loop_end = get_unextended_samples(level, value);
        } else {
            l->legacy_loop_end = value;
        }
    }

    wave_buffer_type get_wave_format(int level) {
        wave_level* l = get_level(level);
        if (!l) return wave_buffer_type_si16;
        if (get_extended() && l->legacy_sample_count > 0) {
            return (wave_buffer_type)l->legacy_sample_ptr[0];
        } else
            return wave_buffer_type_si16;
    }

    void clear();

    bool allocate_level(size_t level, size_t samples, zzub::wave_buffer_type waveFormat, bool stereo);
    bool reallocate_level(size_t level, size_t samples);
    void remove_level(size_t level);
    int get_root_note(size_t level);
    size_t get_samples_per_sec(size_t level) ;
    void set_root_note(size_t level, size_t value);
    void set_samples_per_sec(size_t level, size_t value);
    bool create_wave_range(size_t level, size_t fromSample, size_t numSamples, void** sampleData);
    bool silence_wave_range(size_t level, size_t fromSample, size_t numSamples);
    bool remove_wave_range(size_t level, size_t fromSample, size_t numSamples);
    bool stretch_wave_range(size_t level, size_t fromSample, size_t numSamples, size_t newSize);
    bool insert_wave_at(size_t level, size_t atSample, void* sampleData, size_t channels, int waveFormat, size_t numSamples);
    size_t get_level_index(wave_level* level);
    void set_looping(bool state);
    void set_bidir(bool state);
    bool get_looping();
    bool get_bidir();
    void set_extended();
};

}