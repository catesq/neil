/*
  Copyright (C) 2003-2007 Anders Ervik <calvin@countzero.no>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#pragma once

#include <string>
#include <format>

#include "zzub/plugin.h"
#include "libzzub/metaplugin.h"

float linear_to_dB(float val);
float dB_to_linear(float val);
void handleError(std::string errorTitle);

// wave tools
double square(double v);
double sawtooth(double v);
double triangle(double v);



// #define GET_PLUGIN_AUDIO_FROM(plugin_from, plugin_to, buffer_index) \
//     (plugin_from.last_work_frame != plugin_to.last_work_frame + plugin_to.last_work_buffersize) ? \
//         &plugin_from.callbacks->feedback_buffer[buffer_index].front() : \
//         &plugin_from.work_buffer[buffer_index].front()

// #define PLUGIN_HAS_AUDIO_FROM(plugin, amp) (plugin.last_work_audio_result && amp > 0 && (plugin.last_work_max_left > SIGNAL_TRESHOLD || plugin.last_work_max_right > SIGNAL_TRESHOLD))


namespace zzub {
namespace tools {

std::string describe_zzub_note(uint8_t value);

#define strcmpi strcasecmp



class UnsupportedNumberOfChannels : public std::runtime_error {
public:
    UnsupportedNumberOfChannels(int in, int out) : std::runtime_error(std::format("Unsupported number of channels: in: {}, out: {}", in, out)) {}
};


// used in process_stereo of lv2/vst adapters
// duplicate a output of mono plugin to zzub's stereo or mix down zzub's stereo channel to the input of a mono plugin 
struct CopyChannels {
    virtual void copy(float **src, float **dest, int num_samples) = 0;
    static CopyChannels* build(int num_in, int num_out);
};


struct NullChannels: CopyChannels {
    virtual void copy(float **src, float **dest, int num_samples);
};


struct StereoToMono: CopyChannels {
    virtual void copy(float **src, float **dest, int num_samples);
};


struct MonoToStereo: CopyChannels {
    virtual void copy(float **src, float **dest, int num_samples);
};


struct StereoToStereo: CopyChannels {
    virtual void copy(float **src, float **dest, int num_samples);
};


struct MultiToStereo: CopyChannels {
    int num_src_channels{};
    MultiToStereo(int num_src) : num_src_channels(num_src) {};
    virtual void copy(float **src, float **dest, int num_samples);
};


struct StereoToMulti: CopyChannels {
    int num_dest_channels{};
    StereoToMulti(int num_dest) : num_dest_channels(num_dest) {};
    virtual void copy(float **src, float **dest, int num_samples);
};


// used to find plugins matching the uri name
struct find_info_by_uri {
    std::string uri;
    bool is_versioned = false;
    std::string unversioned_uri{};

    find_info_by_uri(std::string u) {
        uri = u;
        is_versioned = is_versioned_uri(uri);

        if(is_versioned) {
            unversioned_uri = get_deversioned_uri(uri);
        }
    }

    bool is_versioned_uri(std::string uri) {
        return uri.find_first_not_of("0123456789", uri.find_last_of('/') + 1) == std::string::npos;
    }

    std::string get_deversioned_uri(std::string uri) {
        return uri.substr(0, uri.find_last_of('/'));
    }

    bool operator()(const zzub::info* info) {
        if (strcmpi(uri.c_str(), info->uri.c_str()) == 0)
            return true;

        if(is_versioned && is_versioned_uri(info->uri) && strcmpi(unversioned_uri.c_str(), get_deversioned_uri(info->uri).c_str()) == 0)
            return true;

        return false;
    }
};


// get the audio buffer of plugin_from.
// depending where the from and to plugins are in the work order it returns either: 
//   earlier work frame held in the feedback buffer
//   current work frame in work_buffer
// buffer index is 0 or 1
// float* get_plugin_audio_from(zzub::metaplugin& plugin_from, zzub::metaplugin& plugin_to, int buffer_index);

inline float* get_plugin_audio_from(zzub::metaplugin& plugin_from, zzub::metaplugin& plugin_to, int buffer_index) {
    if (plugin_from.last_work_frame != plugin_to.last_work_frame + plugin_to.last_work_buffersize) {
        return &plugin_from.callbacks->feedback_buffer[buffer_index].front();
    } else {
        return &plugin_from.work_buffer[buffer_index].front();
    }
}

inline bool plugin_has_audio(zzub::metaplugin& plugin, float amp) {
    return plugin.last_work_audio_result && amp > 0 && (plugin.last_work_max_left > SIGNAL_TRESHOLD || plugin.last_work_max_right > SIGNAL_TRESHOLD);
}


} // namespace tools
} // namespace zzub


// buffer tools
void AddS2SPanMC(float** output, float** input, int numSamples, float inAmp, float inPan);
void CopyM2S(float *pout, float *pin, int numsamples, float amp);
void Add(float *pout, float *pin, int numsamples, float amp);
void AddStereoToMono(float *pout, float *pin, int numsamples, float amp);
void AddStereoToMono(float *pout, float *pin, int numsamples, float amp, int ch);
void CopyStereoToMono(float *pout, float *pin, int numsamples, float amp);
void Amp(float *pout, int numsamples, float amp);

// disse trenger vi for lavniv� redigering p� flere typer bitformater, waveFormat er buzz-style
// det er kanskje mulig � oppgradere copy-metodene med en interleave p� hver buffer for � gj�re konvertering mellom stereo/mono integrert
void CopyMonoToStereoEx(void* srcbuf, void* targetbuf, size_t numSamples, int waveFormat);
void CopyStereoToMonoEx(void* srcbuf, void* targetbuf, size_t numSamples, int waveFormat);

// from 16 bit conversion
void Copy16To24(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);
void Copy16ToS32(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);
void Copy16ToF32(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);

// from 32 bit floating point conversion
void CopyF32To16(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);
void CopyF32To24(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);
void CopyF32ToS32(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);

// from 32 bit integer conversion
void CopyS32To16(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);
void CopyS32To24(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);
void CopyS32ToF32(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);

// from 24 bit integer conversion
void Copy24To16(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);
void Copy24ToF32(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);
void Copy24ToS32(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);

// trivial conversions
void Copy16(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);
void Copy24(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);
void CopyS32(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);
void CopyF32(void* srcbuf, void* targetbuf, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);

struct S24 {
    union {
        struct {
            char c3[3];
        };
        struct {
            short s;
            char c;
        };
    };
} __attribute__((__packed__));

// auto select based on waveformat
void CopySamples(void *srcbuf, void *targetbuf, size_t numSamples, int srcWaveFormat, int dstWaveFormat, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0);

inline void ConvertSample(const short &src, short &dst) { dst = src; }
inline void ConvertSample(const short &src, S24 &dst) { dst.s = src; dst.c = 0; }
inline void ConvertSample(const short &src, int &dst) { dst = (int)src * (1<<16); }
inline void ConvertSample(const short &src, float &dst) { dst = (float)src / 32767.0f; }

inline void ConvertSample(const S24 &src, short &dst) { dst = src.s; }
inline void ConvertSample(const S24 &src, S24 &dst) { dst = src; }
inline void ConvertSample(const S24 &src, int &dst) { assert(0); }
inline void ConvertSample(const S24 &src, float &dst) { assert(0); }

inline void ConvertSample(const int &src, short &dst) { dst = (short)(src / (1<<16)); }
inline void ConvertSample(const int &src, S24 &dst) { 
    dst.c3[0] = (src & 0x0000ff00) >> 8;
    dst.c3[1] = (src & 0x00ff0000) >> 16;
    dst.c3[2] = (src & 0xff000000) >> 24;
}
inline void ConvertSample(const int &src, int &dst) { dst = src; }
inline void ConvertSample(const int &src, float &dst) { dst = (float)src / 2147483648.0f; }

inline void ConvertSample(const float &src, short &dst) { dst = (short)(std::max(std::min(src,1.0f),-1.0f) * 32767.0f); }
inline void ConvertSample(const float &src, S24 &dst) { 	
    int i = (int)(src * 0x007fffff);
    dst.c3[0] = (i & 0x000000ff);
    dst.c3[1] = (i & 0x0000ff00) >> 8;
    dst.c3[2] = (i & 0x00ff0000) >> 16;
}
inline void ConvertSample(const float &src, int &dst) { dst = (int)(std::max(std::min(src,1.0f),-1.0f) * 2147483648.0f); }
inline void ConvertSample(const float &src, float &dst) { dst = src; }

template <typename srctype, typename dsttype>
inline void CopySamplesT(const srctype *src, dsttype *dst, size_t numSamples, size_t srcstep=1, size_t dststep=1, size_t srcoffset=0, size_t dstoffset=0) {
    src += srcoffset;
    dst += dstoffset;
    while (numSamples--) {
        ConvertSample(*src, *dst);
        src += srcstep;
        dst += dststep;
    }
}

// this is a wrapper for quick GCC support, since GCC's transform() doesnt accept regular tolower
char backslashToSlash(char c);

// zzub tools
int transposeNote(int v, int delta);
int getNoValue(const zzub::parameter* para);
bool validateParameter(int value, const zzub::parameter* p);

// string tools
std::string& trim( std::string& s );
std::string trim( const std::string& s );

// cross platform functions
typedef void *xp_modulehandle;

xp_modulehandle xp_dlopen(const char* path);
void* xp_dlsym(xp_modulehandle handle, const char* symbol);
void xp_dlclose(xp_modulehandle handle);
const char *xp_dlerror();
