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

    event_connection_binding* get_binding(unsigned index) { 
        if(index >= bindings.size())
            return nullptr;
        else
            return &bindings[index]; 
    }

    void remove_binding(event_connection_binding* binding) {
        auto it = bindings.begin();
        while(it != bindings.end()) {
            if(
                it->get_group() == binding->get_group() &&
                it->get_track() == binding->get_track() &&
                it->get_param() == binding->get_param() &&
                it->get_source_param() == binding->get_source_param()
            ) {
                it = bindings.erase(it);
            } else {
                ++it;
            }
        }
    }

    int convert(int value, const zzub::parameter *oldparam, const zzub::parameter *newparam);
    // const zzub::parameter *getParam(struct metaplugin *mp, int group, int index);
};




enum cv_node_type {
    audio_channel = 0, 
    value_param = 1,
    stream_param = 2
};

struct cv_node_audio {
    int channel;
};

// the index of the parameter in the zzub_plugins globals
struct cv_node_param {
    int param;
};

// when node type is audio:
//   input/output channel determined by whether it's the souce or target of the cv_node in cv_port_link
//   channel is 0 or 1 -> the left or right channel
// when node_type is value or stream:
//   param is the index of the zzub_parameter in zzub_plugins globals
struct cv_node {
    cv_node_type type;
    // if this is a audio channel it is either 0 or 1, if it's a value or stream param it's the index of a zzub_parameter in the zzub_plugins globals
    int index;

    bool operator==(const cv_node& other) const { return type == other.type && index == other.index; }
};



struct cv_port_link {
    cv_node source;
    cv_node target;

    bool operator==(const cv_port_link& other) const { return source == other.source && target == other.target; }
};


struct cv_connection : connection {
    std::vector<cv_port_link> port_links;

    cv_connection();
    
    virtual void process_events(zzub::song& player, const zzub::connection_descriptor& conn);
    virtual bool work(zzub::song& player, const zzub::connection_descriptor& conn, int sample_count);

    void add_port_link(const cv_port_link& link);
    void remove_port_link(const cv_port_link& link);
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
