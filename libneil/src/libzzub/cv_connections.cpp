#include "libzzub/cv_connections.h"
#include "libzzub/tools.h"

#include "lanternfish.h"

#include "Gist.h"


namespace zzub {


/******************************** cv_input types ********************************/


/***********************
 *
 * cv_input_audio
 *
 ***********************/



void cv_data_source_audio::work(
    zzub::metaplugin& plugin_from,
    zzub::metaplugin& plugin_to,
    int numsamples
)
{
    switch (node.value) {
    case 1: // left
    case 2: // right
    {
        float* src = &plugin_from.callbacks->feedback_buffer[node.value - 1].front();
        memcpy(data, src, numsamples * sizeof(float));
        break;
    }

    case 3: // stereo
    {
        // pan will be a user controller parameter on cv_node_data - for now it's always 0
        float pan = 0.f;

        float pan_l = 1.0, pan_r = 1.f;

        if (pan < 0.f) {
            pan_r = 1 + pan;
        } else if (pan > 0.f) {
            pan_l = 1 - pan;
        }

        float* left = &plugin_from.callbacks->feedback_buffer[0].front();
        float* right = &plugin_from.callbacks->feedback_buffer[1].front();

        for (int i = 0; i < numsamples; i++) {
            data[i] = left[i] * pan_l + right[i] * pan_r;
        }
    } break;
    }
}


/*****************************
 *
 * cv_input_zzub_param
 *
 *****************************/



// cv_input_zzub_param methods
void cv_data_source_zzub_param::work(
    zzub::metaplugin& from_plugin,
    zzub::metaplugin& to_plugin,
    int numsamples
)
{
    raw_value = from_plugin.callbacks->get_parameter(from_plugin.proxy, param_type, track_index, param_index);
    auto* param_info = from_plugin.callbacks->get_parameter_info(from_plugin.proxy, param_type, param_index);
    norm_value = param_info->normalize(raw_value);
}


/*****************************
 *
 * cv_input_port_param
 *
 *****************************/


// cv_input_zzub_param methods
void cv_data_source_port_param::work(
    zzub::metaplugin& from_plugin,
    zzub::metaplugin& to_plugin,
    int numsamples
)
{
    port_value = source_port->get_value();
}


/*****************************
 *
 * cv_input_port_stream
 *
 *****************************/


// cv_input_zzub_param methods
void cv_data_source_port_stream::work(
    zzub::metaplugin& from_plugin,
    zzub::metaplugin& to_plugin,
    int numsamples
)
{
    source_port->get_value(port_data, numsamples, !is_audio && to_plugin.work_order_index < from_plugin.work_order_index);
}


/******************************** cv_output types ********************************/


/*****************************
 *
 * cv_output_param
 *
 *****************************/

float cv_data_target_param::get_input_value(int numsamples)
{
    switch (data_source->data_type) {
    case cv_data_type::zzub_audio:
        // average audio samples, need to add a flags + settings properties to cv_connector to indicate how to average/amplify -
        // for now just - average(abs(sample))) -> convert to param
        return lanternfish::rms(static_cast<cv_data_source_audio*>(data_source)->data, numsamples);

    case cv_data_type::zzub_param:
        return static_cast<cv_data_source_zzub_param*>(data_source)->norm_value;

    case cv_data_type::cv_param:
        return static_cast<cv_data_source_port_param*>(data_source)->port_value;;

    case cv_data_type::cv_stream:
        return lanternfish::rms(static_cast<cv_data_source_port_stream*>(data_source)->port_data, numsamples);

    default:
        return 0.f;
    }
}



/*****************************
 *
 * cv_output_zzub_param
 *
 *****************************/

void cv_data_target_zzub_param::work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples)
{
    float norm_input = get_input_value(numsamples);

    const zzub::parameter* param = to_plugin.callbacks->get_parameter_info(to_plugin.proxy, param_type, param_index);

    int scaled_output = param->scale(norm_input);

    to_plugin.callbacks->set_parameter(to_plugin.proxy, param_type, track_index, param_index, scaled_output);
}



/*****************************
 *
 * cv_output_port_param
 *
 *****************************/


void cv_data_target_port_param::work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int num_samples)
{
    target_port->set_value(get_input_value(num_samples));
}



/*****************************
 *
 * cv_output_stream
 *
 *****************************/

void cv_data_target_stream::populate_data(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples)
{
    switch (data_source->data_type) {
    case cv_data_type::zzub_audio:
        memcpy(data, static_cast<cv_data_source_audio*>(data_source)->data, numsamples * sizeof(float));
        break;

    case cv_data_type::zzub_param:
        std::fill(data, data + numsamples, static_cast<cv_data_source_zzub_param*>(data_source)->norm_value);
        break;

    case cv_data_type::cv_param:
        std::fill(data, data + numsamples, static_cast<cv_data_source_port_param*>(data_source)->port_value);
        break;

    case cv_data_type::cv_stream:
        memcpy(data, static_cast<cv_data_source_port_stream*>(data_source)->port_data, numsamples * sizeof(float));
    }
}



/*****************************
 *
 * cv_output_port_stream
 *
 *****************************/


void cv_data_target_port_stream::work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples)
{
    populate_data(from_plugin, to_plugin, numsamples);
    target_port->set_value(data, numsamples);
}


/*****************************
 *
 * cv_output_audio
 *
 *****************************/

void cv_data_target_audio::work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples)
{
    populate_data(from_plugin, to_plugin, numsamples);

    float amp = 1.0f;

    bool target_does_input_mixing = (to_plugin.info->flags & zzub::plugin_flag_does_input_mixing) != 0;
    bool result = zzub::tools::plugin_has_audio(from_plugin, amp);
    int sample_count = numsamples;

    if (target_does_input_mixing) {
        bool target_is_bypassed = to_plugin.is_bypassed || (to_plugin.sequencer_state == sequencer_event_type_thru);
        bool target_is_muted = to_plugin.is_muted || (to_plugin.sequencer_state == sequencer_event_type_mute);

        if (result) {
            // the only two plugin which do input mixing, btdsys_ringmod and jmmcd_Crossfade, expect stereo audio so they get the mono input on both channels
            float* stereo_in[2] = { data, data };
            to_plugin.plugin->input(stereo_in, numsamples, amp);
        } else {
            to_plugin.plugin->input(0, 0, 0);
        }

    } else if (result && numsamples > 0) {
        // node.value is -> 0 =left channel, 1 = right channel, 3 = stereo
        switch (node.value) {
        case 1:
        case 2: {
            float* dest = &to_plugin.work_buffer[node.value - 1].front();
            float* src = data;

            do {
                *dest++ += *src++ * amp;
            } while (--numsamples);
        } break;

        case 3: {
            float* dest_l = &to_plugin.work_buffer[0].front();
            float* dest_r = &to_plugin.work_buffer[1].front();
            float* src = data;

            do {
                *dest_l++ += *src * amp;
                *dest_r++ += *src++ * amp;
            } while (--numsamples);
        } break;
        }
    }
}


/*************************************
 *
 * build_cv_input and build_cv_output
 *
 *************************************/


std::shared_ptr<cv_data_source> build_cv_data_source(
    zzub::metaplugin& from_plugin, 
    const cv_node& source, 
    const cv_connector_opts& data
)
{
    auto source_type = static_cast<port_type>(source.port_type);

    bool is_param = source_type == port_type::track || source_type == port_type::param;
    port_flow flow = is_param ? port_flow::input : port_flow::output;

    auto port = from_plugin.plugin->get_port(source_type, flow, source.value);

    
    switch (source_type) {
    case port_type::track:
    case port_type::param:
        if (port)
            return std::make_shared<cv_data_source_port_param>(source, data, port);
        else
            return std::make_shared<cv_data_source_zzub_param>(source, data);

    case port_type::cv:
        if(port)
            return std::make_shared<cv_data_source_port_stream>(source, data, port);
        else {
            printf("cv port problem in data source: port number %d in plugin %s", source.value, from_plugin.info->name);
            assert(false);
        }

        // case midi_track_node:
        //     return std::make_shared<cv_input_midi>(source, data);

    case port_type::audio:
    default:
        return std::make_shared<cv_data_source_audio>(source, data);
    }
}


std::shared_ptr<cv_data_target> build_cv_data_target(
    zzub::metaplugin& to_plugin, 
    const cv_node& target, 
    const cv_connector_opts& data, 
    cv_data_source* input
)
{
    auto port = to_plugin.plugin->get_port(static_cast<port_type>(target.port_type), port_flow::input, target.value);

    switch (static_cast<port_type>(target.port_type)) {
    case port_type::track:
    case port_type::param:
        if(port)
            return std::make_shared<cv_data_target_port_param>(target, data, input, port);
        else
            return std::make_shared<cv_data_target_zzub_param>(target, data, input);

    case port_type::cv:
        if(port)
            return std::make_shared<cv_data_target_port_stream>(target, data, input, port);
        else {
            printf("cv port problem in data target: port number %d in plugin %s", target.value, to_plugin.info->name);
            assert(false);
        }
            

        // case midi_track_node:
        //     return std::make_shared<cv_output_midi_track>(target, data, input);

    case port_type::audio:
    default:
        return std::make_shared<cv_data_target_audio>(target, data, input);
    }
}

}