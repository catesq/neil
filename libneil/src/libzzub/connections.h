#pragma once

#include <vector>

#include "zzub/plugin.h"
#include "graph.h"


namespace zzub {

struct connection {
    connection_type type;
    void* connection_values;
    std::vector<const parameter*> connection_parameters;

    virtual ~connection() {};
    virtual void process_events(zzub::song& player, const connection_descriptor& conn) = 0;
    virtual bool work(zzub::song& player, const connection_descriptor& conn, int sample_count) = 0;

protected:
    // don't instantiate this class directly,
    // use either audio_connection or events_connection or midi_connection or cv_connection
    connection();
};


struct audio_connection_parameter_volume : parameter {
    audio_connection_parameter_volume();
};


struct audio_connection_parameter_panning : parameter {
    audio_connection_parameter_panning();
};


struct audio_connection_values {
    unsigned short amp, pan;
};


struct audio_connection : connection {
    static audio_connection_parameter_volume para_volume;
    static audio_connection_parameter_panning para_panning;

    audio_connection_values values;
    audio_connection_values cvalues;

    float lastFrameMaxSampleL, lastFrameMaxSampleR;

    audio_connection();
    virtual void process_events(zzub::song& player, const zzub::connection_descriptor& conn);
    virtual bool work(zzub::song& player, const zzub::connection_descriptor& conn, int sample_count);
};


struct event_connection_binding {
    int source_param_index;
    int target_group_index;
    int target_track_index;
    int target_param_index;
};



struct event_connection : connection {

    std::vector<event_connection_binding> bindings;

    event_connection();
    virtual void process_events(zzub::song& player, const zzub::connection_descriptor& conn);
    virtual bool work(zzub::song& player, const zzub::connection_descriptor& conn, int sample_count);
    int convert(int value, const zzub::parameter *oldparam, const zzub::parameter *newparam);
    const zzub::parameter *getParam(struct metaplugin *mp, int group, int index);
};


struct midi_connection : connection {

    int device;
    std::string device_name;

    midi_connection();
    int get_midi_device(zzub::song& player, int plugin, std::string name);
    virtual void process_events(zzub::song& player, const zzub::connection_descriptor& conn);
    virtual bool work(zzub::song& player, const zzub::connection_descriptor& conn, int sample_count);
};

} // namespace zzub
