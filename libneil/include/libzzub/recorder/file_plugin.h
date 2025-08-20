#pragma once

#include "zzub/plugin.h"
#include "libzzub/recorder/file.h"

namespace zzub {

struct recorder_file_plugin_info : zzub::info {
    recorder_file_plugin_info();

    virtual zzub::plugin* create_plugin() const;
    virtual bool store_info(zzub::archive *) const { return false; }
};



struct recorder_file_plugin : plugin {
    recorder_file_plugin();

    virtual void destroy();

    virtual void init(zzub::archive *arc);

    virtual void process_events();

    void set_writewave(bool enabled);

    void set_recording(bool enabled);

    virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);

    virtual void stop();

    virtual void command(int index);

    virtual const char * describe_value(int param, int value);

    virtual void configure(const char *key, const char *value);
    virtual void add_input(const char *name, zzub::connection_type type);

private:
    bool writeWave;
    bool autoWrite;
    int ticksWritten;
    bool updateRecording;

    wave_buffer_type format;
    std::string waveFilePath;

    file_recorder recorder;

    struct gvals {
        unsigned char wave;
        unsigned char enable;
    };

    gvals g;
    gvals lg;
    int a[2];
};




}
