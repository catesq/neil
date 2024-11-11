#pragma once


#include "zzub/plugin.h"

namespace zzub {

struct recorder_wavetable_plugin_info : zzub::info {
    recorder_wavetable_plugin_info();
    virtual zzub::plugin* create_plugin() const;
    virtual bool store_info(zzub::archive *) const { return false; }
};





struct recorder_wavetable_plugin : plugin {
    recorder_wavetable_plugin() ;

    void reset_buffers(int size) ;

    void add_buffer();

    virtual void process_events() ;

    virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);

    void write(float** pin, int numsamples);

    int get_buffers_samples() ;

    void flush();

    virtual void stop();

private:
    bool writeWave; // if true, mixed buffers will be written to outfile
    bool autoWrite; // write wave when playing and stop writing when stopped
    int ticksWritten; // number of ticks that have been written

    int buffer_size;
    int buffer_index;
    int buffer_offset;
    std::vector<std::vector<std::vector<float> > > buffers;

    std::string waveName;
    int waveIndex;
    int samples_written;
    wave_buffer_type format;

    bool is_started;

    struct gvals {
        unsigned char wave;
        unsigned char enable;
    };

    gvals g;
    gvals lg;
    int a[2];
};


}