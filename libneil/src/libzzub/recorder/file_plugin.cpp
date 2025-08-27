
#include "libzzub/recorder/file_plugin.h"
#include "libzzub/host.h"
#include "libzzub/song.h"
#include "libzzub/connections.h"
#include "libzzub/player.h"
#include "libzzub/tools.h"

namespace zzub {



recorder_file_plugin_info::recorder_file_plugin_info()
{
    this->flags = zzub::plugin_flag_has_audio_input;
    this->name = "File Recorder";
    this->short_name = "FRecorder";
    this->author = "n/a";
    this->uri = "@zzub.org/recorder/file";
    this->commands = "Set Output File";

    add_global_parameter()
            .set_byte()
            .set_name("File")
            .set_description("Filename")
            .set_value_default(0)
            .set_value_min(0)
            .set_value_max(0)
            .set_state_flag();

    add_global_parameter()
            .set_switch()
            .set_name("Record")
            .set_description("Turn automatic recording on/off")
            .set_value_default(switch_value_off)
            .set_state_flag();

    add_global_parameter()
            .set_switch()
            .set_name("Multitrack")
            .set_description("Stereo or multi track output")
            .set_value_default(switch_value_off)
            .set_state_flag();

    add_attribute()
            .set_name("Record Mode (0=automatic, 1=manual)")
            .set_value_default(0)
            .set_value_min(0)
            .set_value_max(1);

    add_attribute()
            .set_name("Format (0=16bit, 1=32bit float, 2=32bit integer, 3=24bit)")
            .set_value_default(0)
            .set_value_min(0)
            .set_value_max(3);

}


zzub::plugin* 
recorder_file_plugin_info::create_plugin() const 
{
    return new recorder_file_plugin();
}


recorder_file_plugin::recorder_file_plugin() 
  : num_channels(2),
    writeWave(false),
    autoWrite(false),
    ticksWritten(0),
    updateRecording(false),
    recordingMulti(true),
    format(wave_buffer_type_si16),
    waveFilePath(""),
    
    recorder(waveFilePath, format, 44100)
{
    global_values = &g;
    attributes = a;
    lg.enable = 0;
    lg.multitrack = 0;
}



void 
recorder_file_plugin::destroy() {
    // Free all memory in multiChannelInputs
    for (auto& input : channel_buffers) {
        delete[] input;
    }

    channel_buffers.clear();


    stop(); 
    delete this; 
}



void 
recorder_file_plugin::init(zzub::archive *arc)
{
    recorder.set_rate(_master_info->samples_per_second);
}



void 
recorder_file_plugin::add_input(const char *name, zzub::connection_type type)
{
    if(!name || strcmp(name, "Master") != 0) {
        return;
    }

    master = dynamic_cast<master_plugin*>(_host->get_plugin_by_id(0));

    if(!master) {
        return;
    }
}



void 
recorder_file_plugin::process_events() 
{
    autoWrite = (attributes[0] == 0) ? true : false;
    format = (wave_buffer_type)attributes[1];

    if (g.enable != switch_value_none && g.enable != lg.enable) {
        lg.enable = g.enable;
        set_writewave(g.enable);
    }

    if (g.multitrack != switch_value_none && g.multitrack != lg.multitrack) {
        lg.multitrack = g.multitrack;
    }

    if (autoWrite) {
        bool hasWavefile = (waveFilePath.size() > 0);
        bool is_playing = _host->get_state_flags() == state_flag_playing;
        set_writewave(hasWavefile && is_playing);
    }

    if (writeWave) {
        ticksWritten++;
    }
}



// snapshot and cache all audio and cv connections to the master plugin for use in process_stereo()
// the audio connections get L/R stereo tracks, cached in input_plugins 
// cv connections get a mono track, cached in cv_connections 
void
recorder_file_plugin::prepare_recorder()
{
    channel_names = std::vector<std::string>{"Master: left", "Master: right"};

    auto master_proxy = _host->get_metaplugin_by_id(0);
    // recordingMulti = lg.multitrack;

    // if not multi track then exit now
    if (!master_proxy || !recordingMulti) {
        recorder.set_channels(channel_names);
        init_channel_buffers(channel_names.size());
        return;
    }

    int input_count = zzub_plugin_get_input_connection_count(master_proxy);

    // cache plugins for the audio inputs    
    for(int i=0; i < input_count; i++) {
        auto conn = zzub_plugin_get_input_connection(master_proxy, i);

        if(conn && conn->type == connection_type_audio) {
            auto audio_conn = static_cast<audio_connection*>(conn);
            auto plugin_proxy = zzub_plugin_get_input_connection_plugin(master_proxy, input_count);
            audio_inputs.emplace_back(plugin_proxy, &audio_conn->values);

            auto name = plugin_proxy->_player->back.get_plugin(plugin_proxy->id).name;
            channel_names.push_back(name + ": left");
            channel_names.push_back(name + ": right");
        }
    }

    // cache plugins for the cv inputs
    for(const auto& connector : master->recorder_connections) {
        auto src_metaplugin = master_proxy->_player->back.get_plugin(connector.source_node.plugin_id);
        auto node_name = describe_cv_node(&src_metaplugin, &connector.source_node);
        // channel_names.push_back(node_name);
        // recording_connections.push_back()
    }

    recorder.set_channels(channel_names);
    init_channel_buffers(channel_names.size());
    return;
}


void 
recorder_file_plugin::init_channel_buffers(
    int set_num
)
{
    num_channels = set_num;

    // Free all memory in multiChannelInputs
    for (auto& input : channel_buffers) {
        delete[] input;
    }

    channel_buffers.clear();

    // Reallocate with 256 samples per channel
    channel_buffers.resize(num_channels);
    for (int i = 0; i < num_channels; ++i) {
        channel_buffers[i] = new float[zzub_buffer_size];
    }
}


void 
recorder_file_plugin::set_writewave(
    bool enabled
) 
{
    printf("set writewave %s\n", enabled ? "Y":"N");
    if (writeWave == enabled)
        return;
        
    writeWave = enabled;
    updateRecording = true;
}


void 
recorder_file_plugin::set_recording(
    bool enabled
) 
{
    if (!autoWrite)
        return;
    lg.enable = enabled;
    _host->control_change(_host->get_metaplugin(), 1, 0, 1, enabled ? 1 : 0, false, false);
}


std::vector<float*>&
recorder_file_plugin::collect_inputs(
    float **pin,
    int numsamples
) {


    memcpy(channel_buffers[0], pin[0], numsamples);
    memcpy(channel_buffers[1], pin[1], numsamples);
    
    if(!recordingMulti) {
        return channel_buffers;
    }

    int channel_index = 2; 
    
    float* out[2] = { 0, 0 };
    // collect audio inputs
    for(auto &input : audio_inputs) {
        auto& mplugin = input.plugin_proxy->_player->back.get_plugin(input.plugin_proxy->id);

        float* in[2] = {
            &mplugin.work_buffer[0].front(),
            &mplugin.work_buffer[1].front()
        };

        float* out[2] = {
            channel_buffers[channel_index++],
            channel_buffers[channel_index++]
        };

        memset(out[0], 0, numsamples * sizeof(float));
        memset(out[1], 0, numsamples * sizeof(float));

        float amp = input.connection_values->amp / (float) 0x4000;
        float pan = input.connection_values->pan / (float) 0x4000;

        if(zzub::tools::plugin_has_audio(mplugin, amp))
            AddS2SPanMC(out, in, numsamples, amp, pan);
    }

    // collect cv inputs
    for(const auto& connector : cv_inputs) {
        // auto src_metaplugin = _host->get_metaplugin_by_id(connector.source_node.plugin_id);
        // if(!src_metaplugin) {
        //     continue;
        // }

        // auto src_plugin = &src_metaplugin->_player->back.get_plugin(src_metaplugin->id);

        // auto port = src_plugin->get_port(connector.source_node, port_flow_output);
        // if(!port) {
        //     continue;
        // }

        // auto buffer = port->get_buffer();
        // if(!buffer) {
        //     continue;
        // }

        // int samples_to_copy = std::min(numsamples, port->get_buffer_size());
        // if(samples_to_copy > 0) {
        //     auto dest = channel_buffers[channel_index++];
        //     memcpy(dest, buffer, samples_to_copy * sizeof(float));
        // }
    }

    return channel_buffers;
}

bool 
recorder_file_plugin::process_stereo(
    float **pin, 
    float **pout, 
    int numsamples, 
    int mode
) {
    if (updateRecording) {
        set_recording(writeWave);
        updateRecording = false;
    }
    if (writeWave) {
        if (!recorder.is_open()) {
            prepare_recorder();
            recorder.open();
        }
        if (recorder.is_open()) {
            if(lg.multitrack) {
                auto& buffers = collect_inputs(pin, numsamples);
                recorder.write(buffers.data(), numsamples);
            } else {
                recorder.write(pin, numsamples);
            }
        }
    } else if (recorder.is_open()) {
        recorder.close();
    }
    return true;
}



void 
recorder_file_plugin::stop() {
    // if (autoWrite)
    // set_writewave(false);
}


void 
recorder_file_plugin::command(
    int index
) 
{
    if (index == 0) {
        char szFile[260];
        strcpy(szFile, recorder.get_path().c_str());
        printf("GetSaveFileName not implemented!");
    }
}


const char * 
recorder_file_plugin::describe_value(
    int param, 
    int value
) 
{
    switch (param) {
        case 0:
            return recorder.get_path().c_str();
    }
    return 0;
}


void 
recorder_file_plugin::configure(
    const char *key, 
    const char *value
) 
{
    if (!strcmp(key, "wavefilepath")) {
        recorder.set_path(value);
        _host->control_change(_host->get_metaplugin(), 1, 0, 0, 0, false, true);
    }
}


}