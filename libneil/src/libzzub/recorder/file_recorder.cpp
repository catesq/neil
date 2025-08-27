
#include "libzzub/recorder/file_recorder.h"
#include <sstream>

namespace zzub {


bwf_channel_data::bwf_channel_data(
    std::vector<std::string> channel_names
) {


}



void
bwf_channel_data::write(
    SNDFILE* sndfile
) {

}



polywav_channel_data::polywav_channel_data(
    std::vector<std::string> channel_names
) {
    std::ostringstream oss;

    oss << "Channel names: ";

    for (size_t i = 0; i < channel_names.size(); ++i) {
        if (i > 0) oss << ",";
        oss << channel_names[i];
    }

    comment=oss.str();
}



void
polywav_channel_data::write(
    SNDFILE* sf
) {
    sf_set_string(sf, SF_STR_COMMENT, comment.c_str());
}



bool 
file_recorder::open() {
    if (wave_file) 
        return false;

    auto sfinfo = build_info();
    
    wave_file = sf_open(path.c_str(), SFM_WRITE, &sfinfo);

    if (!wave_file) {
        printf("file_recorder.open '%s' failed\n", path.c_str());
        return false;
    }

    sf_samples = new float[zzub::buffer_size * channels];

    // do not delete the channel_data_writer until sf_close_called
    // the ixml string in bwf_channel_data needs to be exist until then
    if(sfinfo.channels > 2) {
        // if(use_ibwf) {
        //      
        // } else {
        channel_data_writer = new polywav_channel_data(channel_names);
        //}
        channel_data_writer->write(wave_file);
    }


    return true;
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
file_recorder::set_channels(
    std::vector<std::string> names
) 
{
    channels = names.size();
    channel_names = names;
}



void 
file_recorder::write(
    float** samples, 
    int num_samples
)
{
    if (!num_samples || !wave_file) 
        return;
        

    float *p = sf_samples;

    for (int i = 0; i < num_samples; i++) {
        for(int c = 0; c < channels; c++) {
            float samp = samples[c][i];
            *p++ = std::clamp(samp, -1.0f, 1.0f);
        }
    }

    sf_writef_float(wave_file, sf_samples, num_samples);
}



void 
file_recorder::close() {
    if (wave_file) {
        printf("file_recorder::close found a wave file\n");
        sf_close(wave_file);

        if(channel_data_writer)
            delete channel_data_writer;

        if(sf_samples)
            delete sf_samples;

        channel_data_writer = nullptr;
        wave_file = nullptr;
        sf_samples = nullptr;
    }
}



bool 
file_recorder::is_open() const {
    return wave_file != nullptr;
}



}