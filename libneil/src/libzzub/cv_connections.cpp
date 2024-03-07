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


void cv_input_audio::process_events(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin) { }

void cv_input_audio::work(zzub::metaplugin& plugin_from, zzub::metaplugin& plugin_to, int numsamples) {
    switch(node.value) {
        case 0:
        case 1: {
            float* src = &plugin_from.callbacks->feedback_buffer[node.value % 2].front();
            memcpy(data, &src[256 - numsamples], numsamples * sizeof(float));
            break;
        }
        
        //  getting input from both channel they have to be mixed
        case 3: {
            // pan will be a user controller parameter on cv_node_data - for now it's always 0
            float pan = 0.f;

            float pan_l = 1.0, pan_r = 1.f;

            if(pan < 0.f) {
                pan_r = 1 + pan;
            } else if (pan > 0.f){
                pan_l = 1 - pan;
            }

            float* left  = &plugin_from.callbacks->feedback_buffer[0].front();
            float* right = &plugin_from.callbacks->feedback_buffer[1].front();
            
            for(int i = 0; i < numsamples; i++) {
                data[i] = left[i] * pan_l + right[i] * pan_r;
            }
        }
    }
    
}


/*****************************
 * 
 * cv_input_zzub_global_param 
 * 
 *****************************/


void cv_input_global_param::process_events(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin) { }

// cv_input_param methods
void cv_input_global_param::work(zzub::metaplugin& from_plugin, zzub::metaplugin& to, int numsamples) {
    raw_value =  from_plugin.callbacks->get_parameter(from_plugin.proxy, zzub_parameter_group_global, 0, node.value);
    norm_value = from_plugin.info->global_parameters[node.value]->normalize(raw_value);
}



/*****************************
 * 
 * cv_input_zzub_track_param 
 * 
 *****************************/


void cv_input_track_param::process_events(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin) { }

// cv_input_param methods
void cv_input_track_param::work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples) {
    raw_value = from_plugin.callbacks->get_parameter(from_plugin.proxy, zzub_parameter_group_track, track_index, param_index);
    norm_value = from_plugin.info->track_parameters[param_index]->normalize(raw_value);
}




/*****************************
 * 
 * cv_input_ext_port 
 * 
 *****************************/


void cv_input_ext_port::process_events(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin) {

}


// cv_input_param methods
void cv_input_ext_port::work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples) {

}




/*****************************
 * 
 * cv_input_midi 
 * 
 *****************************/


void cv_input_midi::process_events(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin) {

}


// cv_input_midi methods
void cv_input_midi::work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples) {

}




/******************************** cv_output types ********************************/





/*****************************
 * 
 * cv_output_global_param 
 * 
 *****************************/


void cv_output_global_param::process_events(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin) {

}

void cv_output_global_param::work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples) {
    switch(input->node.type) {
        case audio_node: {
            //average audio samples, need to add a flags + settings properties to cv_connector to indicate how to average/amplify - 
            // for now just - average(abs(sample))) -> convert to param
            float rms = lanternfish::rms(static_cast<cv_input_audio*>(input)->data, numsamples);
            
            
            break;
        }

        case zzub_track_param_node:
            break;

        case zzub_global_param_node:
            break;

        case ext_port_node:
            break;

        case midi_track_node:
            break;
    }
}


/*****************************
 * 
 * cv_output_track_param 
 * 
 *****************************/


void cv_output_track_param::process_events(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin) {

}

void cv_output_track_param::work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples) {
    switch(input->node.type) {
        case audio_node: {
            //average audio samples, need to add a flags + settings properties to cv_connector to indicate how to average/amplify - 
            // for now just - average(abs(sample))) -> convert to param
            float rms = lanternfish::rms(static_cast<cv_input_audio*>(input)->data, numsamples);
            
            
            break;
        }

        case zzub_track_param_node:
            break;

        case zzub_global_param_node:
            break;

        case ext_port_node:
            break;

        case midi_track_node:
            break;
    }
}


/*****************************
 * 
 * cv_output_ext_port 
 * 
 *****************************/




void cv_output_ext_port::process_events(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin) {

}



void cv_output_ext_port::work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples) {
    switch(input->node.type) {
        case audio_node:
            break;

        case zzub_track_param_node:
            break;

        case zzub_global_param_node:
            break;

        case ext_port_node:
            break;

        case midi_track_node:
            break;
    }
}



/*****************************
 * 
 * cv_output_midi_track 
 * 
 *****************************/



void cv_output_midi_track::process_events(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin) {

}


void cv_output_midi_track::work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples) {
    switch(input->node.type) {
        case audio_node:
            break;

        case zzub_track_param_node:
            break;

        case zzub_global_param_node:
            break;

        case ext_port_node:
            break;

        case midi_track_node:
            break;
    }
}




/*****************************
 * 
 * cv_output_audio 
 * 
 *****************************/



void cv_output_audio::process_events(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin) {
  
}

void cv_output_audio::work(zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples) {
    switch(input->node.type) {
        case audio_node:
            write_buffer(static_cast<cv_input_audio*>(input)->data, from_plugin, to_plugin, numsamples);
            break;

        case zzub_track_param_node:
            write_value_to_buffer(static_cast<cv_input_track_param*>(input)->norm_value, from_plugin, to_plugin, numsamples);
            break;

        case zzub_global_param_node:
            write_value_to_buffer(static_cast<cv_input_global_param*>(input)->norm_value, from_plugin, to_plugin, numsamples);
            break;

        case ext_port_node: 
            write_value_to_buffer(static_cast<cv_input_ext_port*>(input)->value, from_plugin, to_plugin, numsamples);
            break;


        case midi_track_node:
            // return write_buffer(read->data, source, target, numsamples);
            break;
    }
}

void cv_output_audio::write_value_to_buffer(float value, zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples) {
    float *tmp = buf;

    for(int i = 0; i < numsamples; i++) {
        *tmp++ = value;
    }

    write_buffer(buf, from_plugin, to_plugin, numsamples);
}

//    return plugin.last_work_audio_result && amp > 0 && (plugin.last_work_max_left > SIGNAL_TRESHOLD || plugin.last_work_max_right > SIGNAL_TRESHOLD);

void cv_output_audio::write_buffer(float* buffer, zzub::metaplugin& from_plugin, zzub::metaplugin& to_plugin, int numsamples) {
    float amp = 1.0f;

    bool target_does_input_mixing = (to_plugin.info->flags & zzub::plugin_flag_does_input_mixing) != 0;
    bool result = zzub::tools::plugin_has_audio(from_plugin, amp);
    int sample_count = numsamples;
    
    if (target_does_input_mixing) {
        bool target_is_bypassed = to_plugin.is_bypassed || (to_plugin.sequencer_state == sequencer_event_type_thru);
        bool target_is_muted = to_plugin.is_muted || (to_plugin.sequencer_state == sequencer_event_type_mute);

        if(result) {
            // the only two plugin which do input mixing, the btdsys_ringmod and jmmcd_Crossfade, expect stereo audio so they get the mono input on both channels
            float *stereo_in[2] = {buffer, buffer};
            to_plugin.plugin->input(stereo_in, numsamples, amp);
        } else {
            to_plugin.plugin->input(0, 0, 0);
        }

    } else if(result && numsamples > 0) {
        // node.value is -> 0 =left channel, 1 = right channel, 3 = stereo
        switch(node.value) {
            case 0:
            case 1:{
                float* dest = &to_plugin.work_buffer[node.value % 2].front();
                float *src = buffer;

                do {
                    *dest++ += *src++ * amp;

                } while(--numsamples);
            }

            case 3: {
                float* dest_l = &to_plugin.work_buffer[0].front();
                float* dest_r = &to_plugin.work_buffer[1].front();
                float *src = buffer;

                do {
                    *dest_l++ += *src * amp;
                    *dest_r++ += *src++ * amp;

                } while(--numsamples);
            }
        }
    }
}


/*************************************
 * 
 * build_cv_input and build_cv_output 
 * 
 *************************************/


std::shared_ptr<cv_input> build_cv_input(const cv_node& source, const cv_connector_data& data) {
    switch(source.type) {
        case zzub_track_param_node:
            return std::make_shared<cv_input_track_param>(source, data);

        case zzub_global_param_node:
            return std::make_shared<cv_input_global_param>(source, data);

        case ext_port_node:
            return std::make_shared<cv_input_ext_port>(source, data);

        case midi_track_node:
            return std::make_shared<cv_input_midi>(source, data);

        case audio_node:
        default:
            return std::make_shared<cv_input_audio>(source, data);
    }
}


std::shared_ptr<cv_output> build_cv_output(const cv_node& target, const cv_connector_data& data, cv_input* input) {
    switch(target.type) {
        case zzub_track_param_node:
            return std::make_shared<cv_output_track_param>(target, data, input);

        case zzub_global_param_node:
            return std::make_shared<cv_output_global_param>(target, data, input);

        case ext_port_node:
            return std::make_shared<cv_output_ext_port>(target, data, input);

        case midi_track_node:
            return std::make_shared<cv_output_midi_track>(target, data, input);

        case audio_node:
        default:
            return std::make_shared<cv_output_audio>(target, data, input);
    }
}

}