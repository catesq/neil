#pragma once

#include "zzub/plugin.h"
#include "libzzub/connections.h"
#include "libzzub/master.h"
#include "libzzub/recorder/file_recorder.h"

namespace zzub {

struct recorder_file_plugin_info : zzub::info {
    recorder_file_plugin_info();

    virtual zzub::plugin* create_plugin() const;
    virtual bool store_info(zzub::archive *) const { return false; }
};


namespace record {
    struct audio_in {
        metaplugin_proxy* plugin_proxy;
        audio_connection_values* connection_values;
    };

    struct cv_in {

    };
}


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
    void prepare_recorder();
    std::vector<float*>& collect_inputs(float **pin, int numsamples);
    void set_channel_names(std::vector<std::string> names);
    void init_channel_buffers(int channel_count);



    int num_channels;

    bool writeWave;
    bool autoWrite;
    int ticksWritten;
    bool updateRecording;
    bool recordingMulti;

    wave_buffer_type format;
    std::string waveFilePath;

    master_plugin* master;

    // when multi track output enabled this is all audio inputs to the master plugin when recording started
    std::vector<record::audio_in> audio_inputs{};

    // when multi track output enabled this is all cv inputs to the master plugin when the recording started
    std::vector<record::cv_in> cv_inputs{};

    std::vector<float*> channel_buffers{};
    std::vector<std::string> channel_names{};


    // creates the wav file
    file_recorder recorder;

    struct gvals {
        unsigned char wave;
        unsigned char enable;
        unsigned char multitrack;
    };

    gvals g;
    gvals lg;
    int a[2];
};




}
