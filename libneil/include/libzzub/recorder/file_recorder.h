#pragma once

#include "zzub/plugin.h"
#include <sndfile.h>
#include <string>

namespace zzub {


// abstract class used to write channel names to wav file
struct channel_data {
    virtual void write(SNDFILE* wavefile) = 0;
};



// when writing polywav store channel names in an ixml chunk of bwf file 
struct bwf_channel_data : channel_data {
    std::string ixml;

    bwf_channel_data(std::vector<std::string> channel_names);

    void write(SNDFILE* wavefile);
};


// when writing polywav store channel names in a long comment string stored in a riff info ICMT
struct polywav_channel_data : channel_data {
    std::string comment;

    polywav_channel_data(std::vector<std::string> channel_names);

    void write(SNDFILE* wavefile);
};



class file_recorder {
public: 

    uint channels;

    std::string path;

    wave_buffer_type format;

    uint sample_rate;

    std::vector<std::string> channel_names;

    SNDFILE *wave_file;

    // the sample data is received as float[num_channels][buffer_size] sndfile wans float[buffer_size][num_channels]
    float *sf_samples = nullptr;

    channel_data* channel_data_writer = nullptr;

    file_recorder(
        const std::string& path, 
        wave_buffer_type format, 
        uint sample_rate
    )
    : file_recorder(path, format, sample_rate, 2) 
    {
    }


    file_recorder(
        const std::string& path, 
        wave_buffer_type format, 
        uint sample_rate, 
        uint channels
    ) 
    : channels(channels),
      path(path),
      format(format),
      sample_rate(sample_rate),
      wave_file(nullptr)
    {
        for(uint i = 0; i < channels; ++i) {
            channel_names.push_back("Channel " + std::to_string(i + 1));
        }
    }

    
    void set_channels(std::vector<std::string> names);


    void set_path(const std::string& path)
    {
        this->path = path;
    }


    const std::string& get_path()
    {
        return this->path;
    }


    void set_format(wave_buffer_type format)
    {
        this->format = format;
    }


    void set_rate(int sample_rate)
    {
        this->sample_rate = sample_rate;
    }


    bool open();


    void write(
        float** samples, 
        int numSamples
    );


    void close();
    

    bool is_open() const;

private:
    SF_INFO build_info();
};


}