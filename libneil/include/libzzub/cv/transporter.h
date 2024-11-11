#pragma once

#include "libzzub/cv/data.h"
#include "libzzub/cv/source.h"
#include "libzzub/cv/target.h"

#include <vector>

namespace zzub
{





/** 
 * each target port has a data_transporter
 */
struct cv_transporter {
    cv_node target_node;
    cv_data_type data_type;
    std::vector<cv_source*> sources {};
    cv_target* target = nullptr;
    cv_data* data = nullptr;


    cv_transporter(
        cv_node target_node,
        cv_data_type data_type
    ) : target_node(target_node), 
        data_type(data_type)
    {
        data = cv_data::create(data_type);
    }


    ~cv_transporter()
    {
        for(auto source: sources)
            delete source;

        sources.clear();

        delete data;
        delete target;
    }


    void work(
        zzub::metaplugin& from_plugin, 
        zzub::metaplugin& to_plugin, 
        int numsamples,
        bool use_current
    );


    void move_source(
        int from_index,
        int to_index
    );


    void add_source(
        const cv_connector& link, 
        zzub::metaplugin& from,
        zzub::metaplugin& to
    );


    void remove_source(
        const cv_connector& link
    );


    int get_index(
        const cv_connector& link
    );
};





}



















// /*************************************************************************
//  * cv_output_zzub_param and cv_output_port_param read from the input node in the same way
//  * as so cv_output_port_audio and cv_output_port_stream
//  * how they write the data is different
//  ************************************************************************/

// struct cv_data_target_param : public cv_data_target {
//     cv_data_target_param(cv_data_type type, const cv_node& node, const cv_connector_opts& data, cv_data_source* input)
//         : cv_data_target(data_type, node, data, input)
//     {
//     }

// protected:
//     virtual float get_input_value(int numsamples);
// };


// struct cv_data_target_stream : public cv_data_target {
//     using cv_data_target::cv_data_target;

// protected:
//     float data[zzub_buffer_size];
//     void populate_data(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples);
// };


// /*************************************************************************
//  *
//  * cv_output sub types
//  *
//  ************************************************************************/


// /*
//  * cv_output_zzub_param
//  */

// struct cv_data_target_zzub_param : public cv_data_target_param {
//     uint16_t param_type;
//     uint16_t track_index;
//     uint16_t param_index;

//     cv_data_target_zzub_param(
//         const cv_node& node,
//         const cv_connector_opts& data,
//         cv_data_source* input
//     )
//         : cv_data_target_param(cv_data_type::zzub_param, node, data, input)
//     {
//         if (node.port_type == zzub_port_type_track) {
//             param_type = zzub_parameter_group_track;
//             track_index = (node.value >> 16) & 0x00ff;
//             param_index = node.value & 0x00ff;
//         } else {
//             param_type = zzub_parameter_group_global;
//             track_index = 0;
//             param_index = node.value;
//         }
//     }

//     virtual void work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples);
// };


// /*
//  * cv_output_port_param
//  */

// struct cv_data_target_port_param : public cv_data_target_param {

//     cv_data_target_port_param(
//         const cv_node& node,
//         const cv_connector_opts& data,
//         cv_data_source* input,
//         zzub::port* target_port
//     )
//         : cv_data_target_param(cv_data_type::cv_param, node, data, input)
//         , target_port(target_port)
//     {
//     }

//     virtual void work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples);

// protected:
//     zzub::port* target_port;
// };


// /*
//  * cv_output_port_stream
//  */

// struct cv_data_target_port_stream : public cv_data_target_stream {
//     cv_data_target_port_stream(
//         const cv_node& node,
//         const cv_connector_opts& data,
//         cv_data_source* input,
//         zzub::port* target_port
//     )
//         : cv_data_target_stream(cv_data_type::cv_stream, node, data, input)
//         , target_port(target_port)
//     {
//     }

//     virtual void work(
//         zzub::metaplugin& from_plugin,
//         zzub::metaplugin& to_plugin,
//         int numsamples
//     );

// protected:
//     zzub::port* target_port;
// };


// /*
//  * cv_output_audio
//  */

// struct cv_data_target_audio : public cv_data_target_stream {
//     cv_data_target_audio(
//         const cv_node& node,
//         const cv_connector_opts& data,
//         cv_data_source* input
//     )
//         : cv_data_target_stream(cv_data_type::cv_stream, node, data, input)
//     {
//     }

//     virtual void work(
//         zzub::metaplugin& from_plugin,
//         zzub::metaplugin& to_plugin,
//         int numsamples
//     );
// };



