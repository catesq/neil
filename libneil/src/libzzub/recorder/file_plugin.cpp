
#include "libzzub/recorder/file_plugin.h"


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
  : writeWave(false),
    autoWrite(false),
    ticksWritten(0),
    updateRecording(false),
    format(wave_buffer_type_si16),
    waveFilePath(""),
    recorder(waveFilePath, format, 44100)
{
    global_values = &g;
    attributes = a;
    lg.enable = 0;
}


void 
recorder_file_plugin::destroy() {
    stop(); 
    delete this; 
}


void 
recorder_file_plugin::init(zzub::archive *arc)
{
    recorder.set_rate(_master_info->samples_per_second);
}


void 
recorder_file_plugin::process_events() 
{
    autoWrite = (attributes[0] == 0) ? true : false;
    format = (wave_buffer_type)attributes[1];

    if (g.enable != switch_value_none) {
        if (g.enable != lg.enable) {
            lg.enable = g.enable;
            if (g.enable) {
                set_writewave(true);
            } else {
                set_writewave(false);
            }
        }
    }

    if (autoWrite) {
        bool hasWavefile = (waveFilePath.size() > 0);
        if (hasWavefile && (_host->get_state_flags() == state_flag_playing)) {
            set_writewave(true);
        } else {
            set_writewave(false);
        }
    }

    if (writeWave) {
        ticksWritten++;
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
            recorder.open();
        }
        if (recorder.is_open()) {
            recorder.write(pin, numsamples);
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