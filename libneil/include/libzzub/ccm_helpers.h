#pragma once

#include "minizip/mz_compat.h"

#include <string>
#include "zzub/consts.h"
#include "libzzub/wave_info.h"
#include "libzzub/streams.h"
#include "libzzub/player.h"


namespace zzub {


int buzz_to_midi_note(int value);
int midi_to_buzz_note(int value);


int double_to_pan(double v);
double pan_to_double(int pan);
int double_to_amp(double v);
double amp_to_double(int amp);


std::string id_from_ptr(const void *p);
std::string paramtype_to_string(int paramtype);
std::string connectiontype_to_string(int connectiontype);


bool encodeFLAC(zzub::outstream* writer, zzub::wave_info_ex& info, int level);
void decodeFLAC(zzub::instream* reader, zzub::player& player, int wave, int level);



class ArchiveWriter : public zzub::outstream {
    zipFile f;
    std::string currentFileInArchive;
public:
    virtual bool open(std::string fileName);
    virtual bool create(std::string fileName);
    virtual void close();

    void writeLine(std::string line);
    virtual int write(void* v, int size);

    long position() { assert(false); return 0; }
    void seek(long, int) { assert(false); }

    bool createFileInArchive(std::string fileName);
    void closeFileInArchive();
};




struct compressed_file_info {
    std::string name;
    unsigned long compressed_size;
    unsigned long uncompressed_size;
};



class ArchiveReader : public zzub::instream  {
    unzFile f;
    char lastPeekByte;
    bool hasPeeked;

    size_t lastReadOfs;
    //std::string currentFileInArchive;
    compressed_file_info currentFileInfo;
    void populateInfo(compressed_file_info* cfi, std::string fileName, unz_file_info* uzfi);

    void resetFileInArchive();
public:
    virtual bool open(std::string fileName);
    void close();

    bool findFirst(compressed_file_info* info);
    bool findNext(compressed_file_info* info);

    bool openFileInArchive(std::string fileName, compressed_file_info* info=0);
    void closeFileInArchve();

    bool eof();
    char peek();
    virtual void seek(long pos, int mode=SEEK_SET) { assert(false); }
    virtual long position() { return (long)lastReadOfs; }

    virtual int read(void* buffer, int size);
    virtual long size() { return currentFileInfo.uncompressed_size; }
};



class SampleEnumerator {
    unsigned char delta;
    char* buffer;
    unsigned long samples;
    unsigned long currentSample;
    int channels;
    int format;

    float multiplier;
public:
    // maxValue==0x7fff = retreive 16 bit sample values
    // maxValue==0x7fffff = retreive 24 bit sample values
    // maxValue==0x7fffffff = retreive 32 bit sample values
    // maxValue==1 = retreive float sample values
    SampleEnumerator(zzub::wave_info_ex& wave, int level, float maxValue=-1) {
        currentSample=0;

        if (maxValue<0)
            maxValue=static_cast<float>(((((long long)1)<<wave.get_bits_per_sample(level))>>1)-1);

        buffer=(char*)wave.get_sample_ptr(level);
        samples=wave.get_sample_count(level);
        channels=wave.get_stereo()?2:1;
        format=wave.get_wave_format(level);
        switch (format) {
        case zzub::wave_buffer_type_si16:
            delta=2;
            multiplier= maxValue / 0x7fff;
            break;
        case zzub::wave_buffer_type_si24:
            delta=3;
            multiplier= maxValue / 0x7fffff;
            break;
        case zzub::wave_buffer_type_si32:
            delta=4;
            multiplier= maxValue / float(0x7fffffff);
            break;
        case zzub::wave_buffer_type_f32:
            delta=4;
            multiplier= maxValue;
            break;
        default:
            throw "unknown wave format";
        }
    }

    int getInt(char channel) {
        return static_cast<int>(getFloat(channel));
    }

    float getFloat(char channel) {
        float temp;
        ptrdiff_t interleave=delta * channel;
        switch (format) {
        case zzub::wave_buffer_type_si16:
            return static_cast<float>(*(short*)(buffer + interleave)) * multiplier;
        case zzub::wave_buffer_type_si24:
            temp=static_cast<float>(*(unsigned int*)(buffer + interleave));
            return temp * multiplier;
        case zzub::wave_buffer_type_si32:
            return static_cast<float>(*(int*)(buffer + interleave)) * multiplier;
        case zzub::wave_buffer_type_f32:
            return static_cast<float>(*(float*)(buffer + interleave)) * multiplier;
        default:
            throw "unknown wave format";
        }
        return 0;
    }

    inline bool next(int skipSamples=1) {
        int bytes=delta*skipSamples*channels;
        buffer+=bytes;
        currentSample+=skipSamples;
        if (skipSamples<0 || currentSample>=samples) return false;
        return true;
    }


};


}