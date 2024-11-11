#include "libzzub/recorder/wavetable_plugin.h"
#include "libzzub/tools.h"

namespace zzub {

zzub::plugin* recorder_wavetable_plugin_info::create_plugin() const {
    return new recorder_wavetable_plugin();
}


recorder_wavetable_plugin_info::recorder_wavetable_plugin_info() {
    this->flags = zzub::plugin_flag_has_audio_input;
    this->name = "Instrument Recorder";
    this->short_name = "IRecorder";
    this->author = "n/a";
    this->uri = "@zzub.org/recorder/wavetable";

    add_global_parameter()
            .set_wavetable_index()
            .set_name("Instrument")
            .set_description("Instrument to use (01-C8)")
            .set_value_default(wavetable_index_value_min)
            .set_state_flag();

    add_global_parameter()
            .set_switch()
            .set_name("Record")
            .set_description("Turn automatic recording on/off")
            .set_value_default(switch_value_off)
            .set_state_flag();

    add_attribute()
            .set_name("Record Mode (0=wait for play/stop, 1=continous)")
            .set_value_default(0)
            .set_value_min(0)
            .set_value_max(1);

    add_attribute()
            .set_name("Format (0=16bit, 1=32bit float, 2=32bit integer, 3=24bit)")
            .set_value_default(0)
            .set_value_min(0)
            .set_value_max(3);
}






recorder_wavetable_plugin::recorder_wavetable_plugin() {
    writeWave = false;
    autoWrite = false;
    ticksWritten = 0;

    waveIndex = -1;
    samples_written = 0;
    format = wave_buffer_type_si16;
    reset_buffers(zzub_default_rate);	// TODO: do this in init() and use masters samplerate

    global_values = &g;
    attributes = a;
    lg.wave = 0;
    lg.enable = 0;

    is_started = false;
}

void 
recorder_wavetable_plugin::reset_buffers(int size) {
    buffer_size = size;
    buffer_offset = 0;
    buffer_index = 0;
    buffers.clear();
    add_buffer();
}

void 
recorder_wavetable_plugin::add_buffer() {
    buffers.push_back(std::vector<std::vector<float> >());
    buffers.back().resize(2);
    buffers.back()[0].resize(buffer_size);
    buffers.back()[1].resize(buffer_size);
}


void 
recorder_wavetable_plugin::process_events() {
    if (g.wave != wavetable_index_value_none) {
        if (g.wave != lg.wave) {
            lg.wave = g.wave;
            waveIndex = int(g.wave);
            waveName = _host->get_name(_host->get_metaplugin());
        }
    }
    if (g.enable != switch_value_none) {
        if (g.enable != lg.enable) {
            lg.enable = g.enable;
            if (g.enable) {
                format = (wave_buffer_type)attributes[1];
                reset_buffers(_master_info->samples_per_second);
                if (attributes[0] == 0)
                    autoWrite = true;
                else if (attributes[0] == 1)
                    writeWave = true;
            } else {
                autoWrite = false;
                writeWave = false;
            }
        }
    }


    if (autoWrite) {
        if (_host->get_state_flags() == state_flag_playing)
            writeWave = true;
        else
            writeWave = false;
    }

    if (writeWave) {
        ticksWritten++;
    }
}

bool 
recorder_wavetable_plugin::process_stereo(float **pin, float **pout, int numsamples, int mode) {
    if (writeWave) { // shall we write a wavefile?
        if (!is_started) is_started = true;
        if (is_started) write(pin, numsamples);

    } else { // no wave writing
        if (is_started) stop();
    }
    return true;
}

void 
recorder_wavetable_plugin::write(float** pin, int numsamples) {
    if (buffer_offset + numsamples >= buffer_size) {
        int samples_until_end = buffer_size - buffer_offset;
        int samples_at_beginning = numsamples - samples_until_end;
        // fill until end of buffer
        memcpy(&buffers.back()[0][buffer_offset], pin[0], samples_until_end * sizeof(float));
        memcpy(&buffers.back()[1][buffer_offset], pin[1], samples_until_end * sizeof(float));
        buffer_offset += samples_until_end;

        flush();
        add_buffer();

        // put the remainder on the beginning
        memcpy(&buffers.back()[0][0], &pin[0][samples_until_end], samples_at_beginning * sizeof(float));
        memcpy(&buffers.back()[1][0], &pin[1][samples_until_end], samples_at_beginning * sizeof(float));
        buffer_offset = samples_at_beginning;
    } else {
        memcpy(&buffers.back()[0][buffer_offset], pin[0], numsamples * sizeof(float));
        memcpy(&buffers.back()[1][buffer_offset], pin[1], numsamples * sizeof(float));
        buffer_offset += numsamples;
    }
}

int 
recorder_wavetable_plugin::get_buffers_samples() {
    if (buffers.size() == 0) return 0;
    return (buffers.size() - 1) * buffer_size + buffer_offset;
}

void 
recorder_wavetable_plugin::flush() {
    if (!is_started) return ;

    int numsamples = get_buffers_samples();
    std::cout << "allocating buffer with " << numsamples << std::endl;
    _host->allocate_wave_direct(waveIndex, 0, numsamples, format, true, "Recorded");
    // TODO: _host api for non-const samplesw_per_sec.. ooh parameter
    //wave.set_samples_per_sec(0, player->master_info.samples_per_second);

    const wave_level* level = _host->get_wave_level(waveIndex, 0);

    char* wavedata = (char*)level->samples;
    std::vector<std::vector<std::vector<float> > >::iterator i;
    for (i = buffers.begin(); i != buffers.end(); ++i) {
        float* pin[2] = { &(*i)[0].front(), &(*i)[1].front() };
        size_t index = i - buffers.begin();
        int bsize;
        if (index == buffers.size() - 1)
            bsize = buffer_offset; else
            bsize = buffer_size;
        CopySamples(pin[0], wavedata, bsize, wave_buffer_type_f32, format, 1, 2, 0, 0);
        CopySamples(pin[1], wavedata, bsize, wave_buffer_type_f32, format, 1, 2, 0, 1);
        wavedata += bsize * level->get_bytes_per_sample() * 2;
    }

    buffer_offset = 0;
}


void 
recorder_wavetable_plugin::stop() {
    flush();
    is_started = false;
    // set the "Record" parameter to off:
    _host->control_change(_host->get_metaplugin(), 1, 0, 1, 0, false, false);
}



}