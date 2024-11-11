#include "libzzub/cv/source.h"
#include "libzzub/tools.h"
#include "libzzub/metaplugin.h"


namespace zzub {




/**********************************
 *
 * cv_source
 *
 **********************************/

cv_source*
cv_source::create(
    cv_data* data, 
    const cv_node& node, 
    const cv_connector_opts& opts
)
{
    //use data->data_type to build a sublass of cv_source
    switch(data->data_type) {
        case cv_data_type::zzub_audio:
            return new cv_source_mono_audio(data, node, opts);
        case cv_data_type::zzub_param:
            return new cv_source_zzub_param(data, node, opts);
        case cv_data_type::cv_param:
            return new cv_source_port_param(data, node, opts);
        case cv_data_type::cv_stream:
            return new cv_source_port_stream(data, node, opts);
    }

    printf("unknown data type '%d' in cv_source::create\n", data->data_type);
    assert(false);
}







/**********************************
 *
 * cv_source_mono_audio
 *
 **********************************/

void cv_source_mono_audio::initialize(
    zzub::metaplugin& plugin_from
)
{
    src = &plugin_from.callbacks->feedback_buffer[node.value - 1].front();
}

void cv_source_mono_audio::work(
    int numsamples
)
{
    data->add(src, numsamples);
}






/**********************************
 *
 * cv_source_zzub_param
 *
 **********************************/


void cv_source_zzub_param::initialize(
    zzub::metaplugin& from_meta
)
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

    host = from_meta.callbacks;
    param_info = host->get_parameter_info(from_meta.proxy, param_type, param_index);
    plugin_proxy = from_meta.proxy;
}


void cv_source_zzub_param::work(
    int numsamples
)
{
    int raw_value = host->get_parameter(plugin_proxy, param_type, track_index, param_index);
    float norm_value = param_info->normalize(raw_value);

    data->add(norm_value);
}




/**********************************
 *
 * cv_source_port_param
 *
 **********************************/

void cv_source_port_param::initialize(
    zzub::metaplugin& from_meta
)
{
    source_port = from_meta.plugin->get_port(
        static_cast<zzub::port_type>(node.port_type), 
        port_flow::output, 
        node.value
    );
}


void cv_source_port_param::work(
    int numsamples
)
{
    data->add(source_port->get_value());
}


/**********************************
 *
 * cv_source_port_stream
 *
 **********************************/

void cv_source_port_stream::initialize(
    zzub::metaplugin& from_meta
)
{
    source_port = from_meta.plugin->get_port(port_type::param, port_flow::output, node.value);
}


void cv_source_port_stream::work(
    int numsamples
)
{
    data->add(source_port->get_value());
}

}
