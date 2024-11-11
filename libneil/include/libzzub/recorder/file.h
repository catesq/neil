#pragma once

#include "zzub/plugin.h"
#include <sndfile.h>
#include <string>

namespace zzub {


class file_recorder {
public:
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
    : path(path),
      format(format),
      sample_rate(sample_rate),
      channels(channels)
    {
    }


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


    void set_channels(uint channels)
    {
        this->channels = channels;
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

    uint channels;
    std::string path;
    wave_buffer_type format;
    uint sample_rate;
    SNDFILE *wave_file;
};


}