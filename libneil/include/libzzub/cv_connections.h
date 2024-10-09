#pragma once

#include <sys/types.h>

#include "libzzub/metaplugin.h"



namespace zzub {


/*************************************************************************
 * 
 * cv_connector_data
 * 
 ************************************************************************/


enum modulation_mode {
    modulation_add = 0,
    modulation_subtract = 1,
    modulation_multiply = 2,
    modulation_divide = 3,
    modulation_max = 4,
    modulation_min = 5,
    modulation_average = 6,
    modulation_copy = 7
};

struct cv_connector_data {
    float amp = 1.0f;
    uint32_t modulate_mode = 0;
    float offset_before = 0.f;
    float offset_after = 0.f;
};




/*************************************************************************
 * 
 * cv_connector_data
 * 
 ************************************************************************/


enum cv_node_type {
    // the cv_node_type - stored in cv_node.type - changes how the cv_node.value is interpreted
    audio_node = 0,             // value will be 0 or 1 for L/R audio channel
    zzub_global_param_node, // value: index of a zzub plugin global parameter
    zzub_track_param_node,  // value: index of a zzub plugin track parameter
    ext_port_node,         // value: index of a external plugin port - lv2/vst2 or vst3
    // midi_track_node,       // value: copy notes/volume to + from tracks of zzub plugin
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *


// enum cv_node_flags {
//     audio_in = 1,   // read audio from input

//     audio_out = 2,  // read audio from output 

//     midi_track = 128,  // zzub::midi_track or 

//     midi_seq = 256,   // raw midi
// };


/*******************************************************************************************************
 * 
 * cv_node
 * 
 * node->value depends on node->type 
 * 
 * node type = audio:
 *     value is channel 0 or 1 -> left or right 
 * 
 * node_type = zzub_global_param_node:
 *     value is the index of the zzub_parameter in zzub_plugins globals
 * 
 * node_type = zzub_track_param_node:
 *     upper 16 bits of value is track number
 *     lower 16 bits of param is the index of the parameter in that track
 * 
 * node_type = ext_port_node
 *     value is index of the zzub_port (if that plugin supports zzub::port)
 * 
 * node_type is midi_track_node
 *      value is ? - not supporting midi yet
 * 
 *******************************************************************************************************/

union cv_node_value {
    uint32_t channels; // audio channels - bitmask up to 32 channels
    uint32_t index;    // port or param
    struct {
        uint16_t track;
        uint16_t param;
    };
};


struct cv_node {
    int32_t plugin_id = -1;

    uint32_t type;

    uint32_t value;

    // cv_node() : plugin_id(-1), type(audio_node), value({0}) {}
    // cv_node(int32_t plugin_id, uint32_t type, uint32_t value) : plugin_id(plugin_id), type(static_cast<cv_node_type>(type)), value({value}) {}
    
    bool operator==(const cv_node& other) const { return plugin_id == other.plugin_id && type == other.type && value == other.value; }
};



/*******************************************************************************************************
 * 
 * cv_input and cv_output are used in the process_events and work method of cv_connector to move data
 * from source to target plugin
 * 
 * the data will be a either:
 *    a single constant value from a plugin control parameter/port
 *    a stream of 32bit floating point data at the audio rate - for a mono audio channel or cv data
 *    8 bit raw midi stream
 *    zzub data structure - probably midi_track data 
 * 
 *******************************************************************************************************/



struct cv_io {
    virtual void process_events(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin) = 0;
    virtual void work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples) = 0;
    virtual void connected(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin) {}
};



struct cv_input : cv_io {
    cv_node node;
    cv_connector_data data;

    cv_input(const cv_node& node, const cv_connector_data& data) : node(node), data(data) {}
};



struct cv_output : cv_io {
    cv_input* input;
    cv_node node;
    cv_connector_data data;

    cv_output(const cv_node& node, const cv_connector_data& data, cv_input* input) : node(node), data(data), input(input) {}
};



/*************************************************************************
 * 
 * cv_input types
 * 
 ************************************************************************/



struct cv_input_audio : public cv_input {
    float data[256];
    
    using cv_input::cv_input;

    virtual void process_events(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin);
    virtual void work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples);
};




struct cv_input_param : public cv_input  {
    int raw_value;
    float norm_value;

    uint16_t param_type;
    uint16_t track_index;
    uint16_t param_index;

    cv_input_param(const cv_node& node, const cv_connector_data& data) : cv_input(node, data) {
        if(node.type == zzub_track_param_node) {
            param_type = zzub_parameter_group_track;
            track_index = (node.value >> 16) & 0x00ff;
            param_index = node.value & 0x00ff;
        } else {
            param_type = zzub_parameter_group_global;
            track_index = 0;
            param_index = node.value;
        }
    }

    virtual void process_events(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin);
    virtual void work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples);
};




struct cv_input_ext_port : public cv_input  {
    float value;

    using cv_input::cv_input;

    virtual void process_events(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin);
    virtual void work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples);
};



// struct cv_input_midi : cv_input {
//     using cv_input::cv_input;

//     unsigned char data[2048];
//     uint len;

//     virtual void process_events(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin);
//     virtual void work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples);
// };






/*************************************************************************
 * 
 * cv_output types
 * 
 ************************************************************************/

struct cv_output_param : cv_output {
    using cv_output::cv_output;
    
    uint16_t param_type;
    uint16_t track_index;
    uint16_t param_index;

    cv_output_param(const cv_node& node, const cv_connector_data& data, cv_input* input) : cv_output(node, data, input) {
        if(node.type == zzub_track_param_node) {
            param_type = zzub_parameter_group_track;
            track_index = (node.value >> 16) & 0x00ff;
            param_index = node.value & 0x00ff;
        } else {
            param_type = zzub_parameter_group_global;
            track_index = 0;
            param_index = node.value;
        }
    }

    virtual void  process_events(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin);
    virtual void  work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples);

private:
    // between 0.0 and 1.0
    float get_input_value(int numsamples);
};


struct cv_output_ext_port : cv_output {
    using cv_output::cv_output;
    float buf[256];
    zzub::port_type output_type; 
    
    virtual void process_events(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin);
    virtual void work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples);
    virtual void connected(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin);
};



// struct cv_output_midi_track : cv_output {
//     using cv_output::cv_output;

//     virtual void process_events(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin);
//     virtual void work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples);
// };



struct cv_output_audio  : cv_output {
    using cv_output::cv_output;

    // some types of cv_input need a buffer to duplicate a single value   
    float buf[256];

    virtual void process_events(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin);
    virtual void work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples);
    void write_buffer(float* buffer, zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples);
};



/*************************************************************************
 * 
 * cv_output types
 * 
 ************************************************************************/



std::shared_ptr<cv_input> build_cv_input(const cv_node& source, const cv_connector_data& data);
std::shared_ptr<cv_output> build_cv_output(const cv_node& target, const cv_connector_data& data, cv_input* input);



}