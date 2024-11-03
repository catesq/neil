#pragma once

#include <sys/types.h>

#include "libzzub/metaplugin.h"


namespace zzub {


/*************************************************************************
 *
 * cv_connector_opts
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


struct cv_connector_opts {
    float amp = 1.0f;
    uint32_t modulate_mode = 0;
    float offset_before = 0.f;
    float offset_after = 0.f;
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
 * node->value depends on node->port_type
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
    uint32_t index; // port or param
    struct {
        uint16_t track;
        uint16_t param;
    };
};


// represents a link to a plugin parameter, audio or cv port
struct cv_node {
    int32_t plugin_id = -1;

    // zzub::port_type - uses a uint32_t because of hassle with zzub.zidl
    uint32_t port_type;

    uint32_t value;

    // cv_node() : plugin_id(-1), type(audio_node), value({0}) {}
    // cv_node(int32_t plugin_id, uint32_t type, uint32_t value) : plugin_id(plugin_id), type(static_cast<cv_node_type>(type)), value({value}) {}

    bool operator==(const cv_node& other) const { return plugin_id == other.plugin_id && port_type == other.port_type && value == other.value; }
};


enum class cv_data_type {
    zzub_audio, // stereo or mono
    zzub_param,
    cv_param,
    cv_stream // essentially mono audio, difference from zzub_audio data is how zzub audio is stored in metaplugin


};


/***********************************************************************************************************
 *
 * cv_io - used by the cv_connector class to transfer data from source to destination
 *
 * the data is either audio stream or plugin parameter
 * the complexity is the duplicated data types
 * because the zzub_global and zzub track parameters
 * haven't been fully replaced by ports yet
 **********************************************************************************************************/


struct cv_data_transfer {
    cv_data_type data_type;

    cv_data_transfer(cv_data_type data_type)
        : data_type(data_type)
    {
    }

    virtual void process_events(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin) = 0;
    virtual void work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples) = 0;
};


struct cv_data_source : cv_data_transfer {
    cv_node node;
    cv_connector_opts opts;

    cv_data_source(
        cv_data_type data_type,
        const cv_node& node,
        const cv_connector_opts& opts
    )
        : cv_data_transfer(data_type)
        , node(node)
        , opts(opts)
    {
    }

    virtual void process_events(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin) {};
};


struct cv_data_target : cv_data_transfer {
    cv_data_source* data_source;
    cv_node node;
    cv_connector_opts opts;

    cv_data_target(
        cv_data_type data_type,
        const cv_node& node,
        const cv_connector_opts& opts,
        cv_data_source* data_source
    )
        : cv_data_transfer(data_type)
        , node(node)
        , opts(opts)
        , data_source(data_source)
    {
    }

    virtual void process_events(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin) {};

};



/*************************************************************************
 *
 * cv_input types
 *
 ************************************************************************/


struct cv_data_source_audio : public cv_data_source {
    float data[zzub_buffer_size];

    cv_data_source_audio(const cv_node& node, const cv_connector_opts& data)
        : cv_data_source(cv_data_type::zzub_audio, node, data)
    {
    }

    virtual void work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples);
};



/*
 * cv_input_zzub_param
 */

struct cv_data_source_zzub_param : public cv_data_source {
    int raw_value;
    float norm_value;

    uint16_t param_type;
    uint16_t track_index;
    uint16_t param_index;

    cv_data_source_zzub_param(const cv_node& node, const cv_connector_opts& data)
        : cv_data_source(cv_data_type::zzub_param, node, data)
    {
        if (node.port_type == zzub_port_type_track) {
            param_type = zzub_parameter_group_track;
            track_index = (node.value >> 16) & 0x00ff;
            param_index = node.value & 0x00ff;
        } else {
            param_type = zzub_parameter_group_global;
            track_index = 0;
            param_index = node.value;
        }
    }

    virtual void work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples);
};



/*
 * cv_input_port_param
 */

struct cv_data_source_port_param : public cv_data_source {
    float port_value;

    cv_data_source_port_param(const cv_node& node, const cv_connector_opts& data, zzub::port* port)
        : cv_data_source(cv_data_type::cv_param, node, data)
        , source_port(port)
    {
    }

    virtual void work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples);

protected:
    zzub::port* source_port;
};



/*
 * cv_output_port_param
 */

struct cv_data_source_port_stream : public cv_data_source {
    // process_cv is run before process_stereo()
    // when the input is audio the latest data can be used without checking the work order
    bool is_audio;
    float port_data[256];

    cv_data_source_port_stream(const cv_node& node, const cv_connector_opts& data, zzub::port* port)
        : cv_data_source(cv_data_type::cv_stream, node, data)
        , source_port(port)
    {
        is_audio = port->get_type() == zzub::port_type::audio;
    }

    virtual void work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples);

protected:
    zzub::port* source_port;
};



/*************************************************************************
 * cv_output_zzub_param and cv_output_port_param read from the input node in the same way
 * as so cv_output_port_audio and cv_output_port_stream
 * how they write the data is different
 ************************************************************************/

struct cv_data_target_param : public cv_data_target {
    cv_data_target_param(cv_data_type type, const cv_node& node, const cv_connector_opts& data, cv_data_source* input)
        : cv_data_target(data_type, node, data, input)
    {
    }

protected:
    virtual float get_input_value(int numsamples);
};



struct cv_data_target_stream : public cv_data_target {
    using cv_data_target::cv_data_target;

protected:
    float data[zzub_buffer_size];
    void populate_data(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples);
};



/*************************************************************************
 *
 * cv_output sub types
 *
 ************************************************************************/


/*
 * cv_output_zzub_param
 */

struct cv_data_target_zzub_param : public cv_data_target_param {
    uint16_t param_type;
    uint16_t track_index;
    uint16_t param_index;

    cv_data_target_zzub_param(
        const cv_node& node, 
        const cv_connector_opts& data, 
        cv_data_source* input
    )
        : cv_data_target_param(cv_data_type::zzub_param, node, data, input)
    {
        if (node.port_type == zzub_port_type_track) {
            param_type = zzub_parameter_group_track;
            track_index = (node.value >> 16) & 0x00ff;
            param_index = node.value & 0x00ff;
        } else {
            param_type = zzub_parameter_group_global;
            track_index = 0;
            param_index = node.value;
        }
    }

    virtual void work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples);
};



/*
 * cv_output_port_param
 */

struct cv_data_target_port_param : public cv_data_target_param {

    cv_data_target_port_param(
        const cv_node& node, 
        const cv_connector_opts& data, 
        cv_data_source* input, 
        zzub::port* target_port
    )
        : cv_data_target_param(cv_data_type::cv_param, node, data, input), target_port(target_port)
    {
    }

    virtual void work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples);

protected:
    zzub::port* target_port;
};






/*
 * cv_output_port_stream
 */

struct cv_data_target_port_stream : public cv_data_target_stream {
    cv_data_target_port_stream(
        const cv_node& node, 
        const cv_connector_opts& data, 
        cv_data_source* input, 
    zzub::port* target_port
    )
        : cv_data_target_stream(cv_data_type::cv_stream, node, data, input), target_port(target_port)
    {
    }

    virtual void work(
        zzub::metaplugin& from_plugin, 
        zzub::metaplugin& to_plugin, 
        int numsamples
    );

protected:
    zzub::port* target_port;
};



/*
 * cv_output_audio
 */

struct cv_data_target_audio : public cv_data_target_stream {
    cv_data_target_audio(
        const cv_node& node, 
        const cv_connector_opts& data, 
        cv_data_source* input
    )
        : cv_data_target_stream(cv_data_type::cv_stream, node, data, input)
    {
    }

    virtual void work(
        zzub::metaplugin& from_plugin, 
        zzub::metaplugin& to_plugin, 
        int numsamples
    );
};




/*
 * builders for the cv_input and cv_output - used in cv_connector::connected
 */

std::shared_ptr<cv_data_source> build_cv_data_source(
    zzub::metaplugin& from_plugin, 
    const cv_node& source, 
    const cv_connector_opts& opts
);


std::shared_ptr<cv_data_target> build_cv_data_target(
    zzub::metaplugin& from_plugin, 
    const cv_node& target, 
    const cv_connector_opts& opts, cv_data_source* data_source
);



}