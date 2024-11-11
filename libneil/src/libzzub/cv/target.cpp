#include "libzzub/cv/target.h"
#include "libzzub/cv/data.h"

namespace zzub {

cv_target*
cv_target::create(
    cv_data* data, 
    const cv_node& node,
    const cv_connector_opts& opts
) {
    switch(data->data_type) {
        case cv_data_type::zzub_audio:
            return new cv_target_zzub_audio(data, node, opts);

        case cv_data_type::zzub_param:
            return new cv_target_zzub_param(data, node, opts);

        case cv_data_type::cv_param:
            return new cv_target_port_param(data, node, opts);

        case cv_data_type::cv_stream:
            return new cv_target_port_stream(data, node, opts);
    }

    printf("unknown data type in cv_target::create()\n");
    assert(false);
}




/**
 * 
 */

void
cv_target_zzub_audio::initialize(
    zzub::metaplugin& meta
) {

}


void
cv_target_zzub_audio::send(
    int numsamples
)  {

}



/**
 * 
 */

void
cv_target_zzub_param::initialize(
    zzub::metaplugin& meta
) {
    if (node.port_type == zzub_port_type_track) {
        param_type = zzub_parameter_group_track;
        track_index = (node.value >> 16) & 0x00ff;
        param_index = node.value & 0x00ff;
    } else {
        param_type = zzub_parameter_group_global;
        track_index = 0;
        param_index = node.value;
    }

    host = meta.callbacks;
    param_info = host->get_parameter_info(meta.proxy, param_type, param_index);
}


void
cv_target_zzub_param::send(
    int numsamples
)  {
    float norm_input = param_info->scale(data->get());
    host->set_parameter(plugin_proxy, param_type, track_index, param_index, norm_input);
}




/**
 * 
 */

void
cv_target_port_param::initialize(
    zzub::metaplugin& meta
) {
    target_port = meta.plugin->get_port(
        static_cast<zzub::port_type>(node.port_type), 
        port_flow::input, 
        node.value
    );
}


void
cv_target_port_param::send(
    int numsamples
)  {
    target_port->set_value(data->get());
}





/**
 * 
 */

void
cv_target_port_stream::initialize(
    zzub::metaplugin& meta
) {
    target_port = meta.plugin->get_port(
        static_cast<zzub::port_type>(node.port_type), 
        port_flow::input, 
        node.value
    );
}


void
cv_target_port_stream::send(
    int numsamples
)  {
    target_port->set_value(data->get(numsamples), numsamples);
}




}