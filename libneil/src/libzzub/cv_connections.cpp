#include "libzzub/cv_connections.h"
#include "libzzub/tools.h"


namespace zzub {
    

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *


bool cv_input_audio::read(zzub::metaplugin& plugin_from, zzub::metaplugin& plugin_to, int numsamples) {
    float* src = zzub::tools::get_plugin_audio_from(plugin_from, plugin_to, node.value % 1);

    memcpy(data, src, numsamples * sizeof(float));  

    return true;
}


// cv_input_param methods
bool cv_input_zzub_param::read(zzub::metaplugin& source, zzub::metaplugin& target, int numsamples) {
    return true;
}


// cv_input_param methods
bool cv_input_ext_port::read(zzub::metaplugin& source, zzub::metaplugin& target, int numsamples) {
    return true;
}


// cv_input_midi methods
bool cv_input_midi::read(zzub::metaplugin& source, zzub::metaplugin& target, int numsamples) {
    return true;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *


// cv_target_param methods
bool cv_output_zzub_param::write(cv_input* read, zzub::metaplugin& source, zzub::metaplugin& target, int numsamples) {
    switch(read->node.type) {
        case audio_node:{
            //average audio samples, need to add a flags + settings properties to cv_connector to indicate how to average/amplify - 
            // for now just - average(abs(sample))) -> convert to param
            float *src = static_cast<cv_input_audio*>(read)->data;

            float sum = 0.0f;
            for(int i = 0; i < numsamples; i++) {
                sum += fabsf(*src++);
            }
            
            return true;
        }

        case zzub_param_node:
            return true;

        case ext_port_node:
            return true;

        case midi_track_node:
            return true;
    }

    return true;
}



bool cv_output_ext_port::write(cv_input* read, zzub::metaplugin& source, zzub::metaplugin& target, int numsamples) {
    switch(read->node.type) {
        case audio_node:
            return true;

        case zzub_param_node:
            return true;

        case ext_port_node:
            return true;

        case midi_track_node:
            return true;
    }

    return true;
}



bool cv_output_midi_track::write(cv_input* read, zzub::metaplugin& source, zzub::metaplugin& target, int numsamples) {
    switch(read->node.type) {
        case audio_node:
            return true;

        case zzub_param_node:
            return true;

        case ext_port_node:
            return true;

        case midi_track_node:
            return true;
    }

    return true;
}



bool cv_output_audio::write(cv_input* read, zzub::metaplugin& source, zzub::metaplugin& target, int numsamples) {
    switch(read->node.type) {
        case audio_node:
            return write_buffer(static_cast<cv_input_audio*>(read)->data, source, target, numsamples);

        case zzub_param_node: {
            int param_int = source.callbacks->get_parameter(source.proxy, zzub_parameter_group_global, 0, read->node.value);
            float param_norm = source.info->global_parameters[read->node.value]->normalize(param_int);
            float *data = buf;

            for(int i = 0; i < numsamples; i++) {
                *data++ = param_norm;
            }

            return write_buffer(data, source, target, numsamples);
        }

        case ext_port_node: {
            zzub::port* port = source.plugin->get_port(read->node.value);
            float value = port->get_value();
            float *data = buf;

            for(int i = 0; i < numsamples; i++) {
                *data++ = value;
            }

            return write_buffer(data, source, target, numsamples);
        }

        case midi_track_node:
            // return write_buffer(read->data, source, target, numsamples);
            return true;
    }
    
    return true;
}


// cv_target_audio methods
bool cv_output_audio::write_buffer(float* data, zzub::metaplugin& source, zzub::metaplugin& target, int numsamples) {
    float amp = 1.0f;

    bool target_does_input_mixing = (target.info->flags & zzub::plugin_flag_does_input_mixing) != 0;
    bool result = zzub::tools::plugin_has_audio(source, amp);

    if (target_does_input_mixing) {
        bool target_to_is_bypassed = target.is_bypassed || (target.sequencer_state == sequencer_event_type_thru);
        bool target_to_is_muted = target.is_muted || (target.sequencer_state == sequencer_event_type_mute);

        if(result) {
            // the only two plugin which do input mixing, the btdsys_ringmod and jmmcd_Crossfade, expect stereo audio so they get the mono input on both channels
            float *stereo_in[2] = {data, data};
            target.plugin->input(stereo_in, numsamples, amp);
        } else {
            target.plugin->input(0, 0, 0);
        }

    } else if(result && numsamples > 0) {
        float* dest = &target.work_buffer[node.value % 1].front();
        float *src = data;

        do {
            *dest++ += *src++ * amp;
        } while(--numsamples);

    } else {
        return false;
    }

    return true;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

std::shared_ptr<cv_input> build_cv_input(const cv_node& source) {
    printf("build input node type: %d\n", source.type);
    switch(source.type) {
        case zzub_param_node:
            return std::make_shared<cv_input_zzub_param>(source);

        case ext_port_node:
            return std::make_shared<cv_input_ext_port>(source);

        case midi_track_node:
            return std::make_shared<cv_input_midi>(source);

        case audio_node:
        default:
            return std::make_shared<cv_input_audio>(source);
    }
}


std::shared_ptr<cv_output> build_cv_output(const cv_node& target) {
    printf("build output node type: %d\n", target.type);
    switch(target.type) {
        case zzub_param_node:
            return std::make_shared<cv_output_zzub_param>(target);

        case ext_port_node:
            return std::make_shared<cv_output_ext_port>(target);

        case midi_track_node:
            return std::make_shared<cv_output_midi_track>(target);

        case audio_node:
        default:
            return std::make_shared<cv_output_audio>(target);
    }
}

}