#pragma once

#include <vector>
#include <memory>

#include "zzub/zzub_data.h"
#include "zzub/plugin.h"
#include "graph.h"

#include "cv_connections.h"



namespace zzub {

struct event_connection;
struct audio_connection;
struct cv_connection;
struct midi_connection;


struct connection {
    connection_type type;
    void* connection_values;
    std::vector<const parameter*> connection_parameters;

    virtual         ~connection() {};
    virtual void    process_events(zzub::song& player, const connection_descriptor& conn) = 0;
    virtual bool    work(zzub::song& player, const connection_descriptor& conn, int sample_count) = 0;

    
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



struct cv_connector {
    cv_node source;
    cv_node target;

    std::shared_ptr<cv_input> input;
    std::shared_ptr<cv_output> output;

    cv_connector(cv_node source, cv_node target) : source(source), target(target) {
        input = build_cv_input(this->source);
        output = build_cv_output(this->target);
        std::cout << "cv_connector::cv_connector source: " << source.type <<  ", input: " <<  input->node.type <<  ", value: " <<  input->node.value << ", plugin_id: " <<  input->node.plugin_id << std::endl;
        std::cout << "cv_connector::cv_connector target: " << target.type <<  ", output: " <<  output->node.type <<  ", value: " <<  output->node.value << ", plugin_id: " <<  output->node.plugin_id << std::endl;
    }

    bool work(zzub::song& player, zzub::metaplugin& from, zzub::metaplugin& to, int sample_count) {
        if(input->read(from, to, sample_count)) {
            return output->write(input.get(), from, to, sample_count);
        }

        return false;
    }

    bool operator==(const cv_connector& other) const { return source == other.source && target == other.target; }
};


struct cv_connection : connection {
    std::vector<cv_connector> connectors;
    int count = 0;

    cv_connection();
    
    virtual void process_events(zzub::song& player, const zzub::connection_descriptor& conn);
    virtual bool work(zzub::song& player, const zzub::connection_descriptor& conn, int sample_count);

    void add_connector(const cv_connector& link);
    void remove_connector(const cv_connector& link);
    bool has_connector(const cv_connector& link);
    int  num_connectors() const { return connectors.size(); }
};


struct midi_connection : connection {
    int device;
    std::string device_name;

    midi_connection();
    int          get_midi_device(zzub::song& player, int plugin, std::string name);
    virtual void process_events(zzub::song& player, const zzub::connection_descriptor& conn);
    virtual bool work(zzub::song& player, const zzub::connection_descriptor& conn, int sample_count);
};


} // namespace zzub
