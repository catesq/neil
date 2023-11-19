#pragma once

#include <vector>

#include "zzub/zzub_data.h"
#include "zzub/plugin.h"
#include "graph.h"


namespace zzub {

struct event_connection;
struct audio_connection;
struct cv_connection;
struct midi_connection;

struct connection {
    connection_type type;
    void* connection_values;
    std::vector<const parameter*> connection_parameters;

    virtual ~connection() {};
    virtual void process_events(zzub::song& player, const connection_descriptor& conn) = 0;
    virtual bool work(zzub::song& player, const connection_descriptor& conn, int sample_count) = 0;

    // cast to the correct subtype - was quickest way to give python access to the right subtypes of connection 
    connection_type get_type() const { return type; }

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

    int get_group() const { return target_group_index; }
    int get_track() const { return target_track_index; }
    int get_param() const { return target_param_index; }
    int get_source_param() const { return source_param_index; }
};



struct event_connection : connection {

    std::vector<event_connection_binding> bindings;

    event_connection();
    virtual void process_events(zzub::song& player, const zzub::connection_descriptor& conn);
    virtual bool work(zzub::song& player, const zzub::connection_descriptor& conn, int sample_count);

    int get_binding_count() const { return bindings.size(); }

    event_connection_binding* get_binding(int index) { 
        if(index < 0 || index >= bindings.size())
            return nullptr;
        else
            return &bindings[index]; 
    }

    int convert(int value, const zzub::parameter *oldparam, const zzub::parameter *newparam);
    // const zzub::parameter *getParam(struct metaplugin *mp, int group, int index);
};


namespace connections {
    // channel is either left or right channel of a plugin
    // the input/output is determined by if it's the souce or target link
    enum link_type {
        audio = 0, 
        parameter = 1
    };

    struct audio_link {
        int channel;
    };

    struct global_param_link {
        int param;
    };

    struct link {
        zzub_parameter_group type;
        union {
            audio_link audio;
            global_param_link global;
        };
    };
};


struct cv_port_link {
    connections::link source;
    connections::link target;

    float buffer[zzub_buffer_size];
};


struct cv_connection : connection {
    std::vector<cv_port_link> port_links;

    cv_connection();
    
    virtual void process_events(zzub::song& player, const zzub::connection_descriptor& conn);
    virtual bool work(zzub::song& player, const zzub::connection_descriptor& conn, int sample_count);

    virtual void add_port_link(const cv_port_link& link) { port_links.push_back(link); }
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
