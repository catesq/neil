
#include "libzzub/recorder/file.h"

namespace zzub {



bool 
file_recorder::open() {
    if (wave_file) 
        return false;

    auto sfinfo = build_info();
    
    wave_file = sf_open(path.c_str(), SFM_WRITE, &sfinfo);

    if (wave_file)
        return true;

    printf("file_recorder.open '%s' failed\n", path.c_str());

    return false;
}


SF_INFO
file_recorder::build_info()
{
    SF_INFO sfinfo;
    memset(&sfinfo, 0, sizeof(sfinfo));

    sfinfo.samplerate = sample_rate;
    sfinfo.channels = channels;
    sfinfo.format = SF_FORMAT_WAV;

    switch (format) {
        case wave_buffer_type_si16:
            sfinfo.format |= SF_FORMAT_PCM_16;
            break;
        case wave_buffer_type_si24:
            sfinfo.format |= SF_FORMAT_PCM_24;
            break;
        case wave_buffer_type_si32:
            sfinfo.format |= SF_FORMAT_PCM_32;
            break;
        case wave_buffer_type_f32:
            sfinfo.format |= SF_FORMAT_FLOAT;
            break;
        default:
            sfinfo.format |= SF_FORMAT_PCM_16;
            break;
    }

    return sfinfo;
}


void 
file_recorder::write(float** samples, int num_samples) {
    if (!num_samples || !wave_file) 
        return;
        
    float ilsamples[zzub::buffer_size * channels];
    float *p = ilsamples;

    for (int i = 0; i < num_samples; i++) {
        for(int c = 0; c < channels; c++) {
            float samp = samples[c][i];
            *p++ = std::clamp(samp, -1.0f, 1.0f);
        }
    }

    sf_writef_float(wave_file, ilsamples, num_samples);
}

void 
file_recorder::close() {
    if (wave_file) {
        printf("file_recorder::close found a wave file\n");
        sf_close(wave_file);
        wave_file = nullptr;
    }
}


bool 
file_recorder::is_open() const {
    return wave_file != nullptr;
}



}