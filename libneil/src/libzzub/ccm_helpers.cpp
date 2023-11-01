

#include "libzzub/ccm_helpers.h"
#include <FLAC/all.h>
#include "libzzub/wavetable.h"


#if defined(_MAX_PATH)
#define CCM_MAX_PATH MAX_PATH
#else
#define CCM_MAX_PATH 32768
#endif

#define CHUNK_OF_SAMPLES 2048



namespace zzub {


double amp_to_double(int amp) {
    return double(amp) / 16384.0;
}

int double_to_amp(double v) {
    return int((v * 16384.0) + 0.5);
}

double pan_to_double(int pan) {
    return (double(pan) / 16384.0) - 1.0;
}

int double_to_pan(double v) {
    return int(((v + 1.0) * 16384.0) + 0.5);
}

int midi_to_buzz_note(int value) {
    return ((value / 12) << 4) + (value % 12) + 1;
}

int buzz_to_midi_note(int value) {
    return 12 * (value >> 4) + (value & 0xf) - 1;
}

std::string connectiontype_to_string(int connectiontype) {
    switch (connectiontype) {
    case zzub::connection_type_audio: return "audio";
    case zzub::connection_type_event: return "event";
    case zzub::connection_type_midi: return "midi";
    default: assert(0);
    }
    return "";
}

std::string paramtype_to_string(int paramtype) {
    switch (paramtype) {
    case zzub::parameter_type_note:
        return "note16"; // buzz note with base 16
    case zzub::parameter_type_switch:
        return "switch";
    case zzub::parameter_type_byte:
        return "byte";
    case zzub::parameter_type_word:
        return "word";
    default:
        assert(0);
    }
    return "";
}


std::string id_from_ptr(const void *p) {
    char id[64];
    sprintf(id, "%p", p);
    return id;
}


/***

    ArchiveWriter

***/
bool ArchiveWriter::create(std::string fileName) {
    // truncate file to 0 bytes with a FileWriter:
    zzub::file_outstream reset;
    if (!reset.create(fileName.c_str())) return false;
    reset.close();

    // open zip and append at end of file:
    f=zipOpen(fileName.c_str(), APPEND_STATUS_CREATEAFTER);
    if (!f)
        return false;

    return true;
}

bool ArchiveWriter::open(std::string fileName) {
    f=zipOpen(fileName.c_str(), APPEND_STATUS_ADDINZIP);
    if (!f) {
        f=zipOpen(fileName.c_str(), APPEND_STATUS_CREATE);
        
        if (!f)
            return false;
    }

    return true;
}

void ArchiveWriter::close() {
    zipClose(f, 0);
}

int ArchiveWriter::write(void* v, int size) {
    if (ZIP_OK!=zipWriteInFileInZip(f, v, size)) {
        // no error reporting

    }
    return size;
}

bool ArchiveWriter::createFileInArchive(std::string fileName) {
    if (currentFileInArchive!="") {
        closeFileInArchive();
    }

    if (MZ_OK!=zipOpenNewFileInZip(f, fileName.c_str(), 0, 0, 0, 0, 0, 0, MZ_COMPRESS_METHOD_DEFLATE, MZ_ZIP_FLAG_DEFLATE_NORMAL))
        return false;

    currentFileInArchive=fileName;

    return true;
}

void ArchiveWriter::closeFileInArchive() {
    if (currentFileInArchive=="") return ;
    zipCloseFileInZip(f);
    currentFileInArchive="";
}

void ArchiveWriter::writeLine(std::string line) {
    line += "\n";
    outstream::write(line.c_str());
}

/***

    ArchiveReader

***/

bool ArchiveReader::open(std::string fileName) {
    f=unzOpen(fileName.c_str());

    if (!f) return false;

    resetFileInArchive();
    return true;
}

void ArchiveReader::close() {
    unzClose(f);
}

void ArchiveReader::populateInfo(compressed_file_info* cfi, std::string fileName, unz_file_info* uzfi) {
    cfi->name=fileName;
    cfi->compressed_size=uzfi->compressed_size;
    cfi->uncompressed_size=uzfi->uncompressed_size;
}

bool ArchiveReader::findFirst(compressed_file_info* info) {
    if (UNZ_OK!=unzGoToFirstFile(f)) return false;

    unz_file_info file_info;
    char fileNameInZip[CCM_MAX_PATH];

    int err = unzGetCurrentFileInfo(f, &file_info, fileNameInZip, CCM_MAX_PATH, NULL, 0, NULL, 0);
    if (err!=UNZ_OK) return false;

    if (info) populateInfo(info, fileNameInZip, &file_info);
    return true;
}

bool ArchiveReader::findNext(compressed_file_info* info) {
    if (UNZ_OK!=unzGoToNextFile(f)) return false;

    unz_file_info file_info;
    char fileNameInZip[CCM_MAX_PATH];

    int err = unzGetCurrentFileInfo(f, &file_info, fileNameInZip, CCM_MAX_PATH, NULL, 0, NULL, 0);
    if (err!=UNZ_OK) return false;

    if (info) populateInfo(info, fileNameInZip, &file_info);
    return true;
}

#define CASESENSITIVITY (0)

bool ArchiveReader::openFileInArchive(std::string fileName, compressed_file_info* info) {
    if (unzLocateFile(f, fileName.c_str(), CASESENSITIVITY)!=UNZ_OK) return false;

    unz_file_info file_info;
    char fileNameInZip[32768];

    int err = unzGetCurrentFileInfo(f, &file_info, fileNameInZip, 32768, NULL, 0, NULL, 0);
    if (err!=UNZ_OK) return false;

    if (UNZ_OK!=unzOpenCurrentFile(f)) return false;

    populateInfo(&currentFileInfo, fileNameInZip, &file_info);

    if (info) *info=currentFileInfo;

    return true;
}

void ArchiveReader::closeFileInArchve() {
    if (currentFileInfo.name=="")
        return;

    unzCloseCurrentFile(f);
    resetFileInArchive();
}

int ArchiveReader::read(void* buffer, int size) {
    if (currentFileInfo.name=="") return 0;

    char* charBuffer = (char*)buffer;
    size_t peekSize = 0;
    if (hasPeeked && size > 0) {
        charBuffer[0] = lastPeekByte;
        charBuffer++;
        size--;
        lastReadOfs++;
        hasPeeked = false;
        peekSize = 1;
    }

    int err = unzReadCurrentFile(f, charBuffer, size);
    if (err < 0)
        return 0;
    err += (int)peekSize;
    lastReadOfs += err;
    return err;
}

void ArchiveReader::resetFileInArchive() {
    hasPeeked = false;
    currentFileInfo.name = "";
    lastReadOfs = 0;
}



// Flac encoder


static FLAC__StreamEncoderWriteStatus flac_stream_encoder_write_callback(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame, void *client_data) {
    zzub::outstream* writer=(zzub::outstream*)client_data;

    writer->write((void*)buffer, sizeof(FLAC__byte)*bytes);
    return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
    //return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
}

void flac_stream_encoder_metadata_callback(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetadata *metadata, void *client_data) {
    /*
     * Nothing to do; if we get here, we're decoding to stdout, in
     * which case we can't seek backwards to write new metadata.
     */
    (void)encoder, (void)metadata, (void)client_data;
}

bool encodeFLAC(zzub::outstream* writer, zzub::wave_info_ex& info, int level) {
    int channels = info.get_stereo() ? 2 : 1;
    // flac is not going to encode anything that's not
    // having a standard samplerate, and since that
    // doesn't matter anyway, pick the default one.
    int sample_rate = info.get_samples_per_sec(level);
    int bps = info.get_bits_per_sample(level);
    int num_samples = info.get_sample_count(level);


    FLAC__StreamEncoder *stream = FLAC__stream_encoder_new();

    FLAC__stream_encoder_set_channels(stream, channels);
    FLAC__stream_encoder_set_bits_per_sample(stream, bps);
    FLAC__stream_encoder_set_sample_rate(stream, sample_rate);
    FLAC__stream_encoder_set_total_samples_estimate(stream, num_samples);
    int result =
            FLAC__stream_encoder_init_stream(stream,
                                             flac_stream_encoder_write_callback,
                                             NULL,
                                             NULL,
                                             flac_stream_encoder_metadata_callback,
                                             writer);
    // if this fails, we want it to crash hard - or else will cause dataloss
    assert(result == FLAC__STREAM_ENCODER_OK);
    FLAC__int32 buffer[FLAC__MAX_CHANNELS][CHUNK_OF_SAMPLES];
    FLAC__int32* input_[FLAC__MAX_CHANNELS];

    for(unsigned int i = 0; i < FLAC__MAX_CHANNELS; i++)
        input_[i] = &(buffer[i][0]);

    bool done=false;
    //    unsigned int ofs=0;
    
    zzub::SampleEnumerator samples(info, level, -1);

    while (!done) {
        unsigned int len=0;
        for (int i=0; i<CHUNK_OF_SAMPLES; i++) {
            buffer[0][len]=samples.getInt(0);
            if (channels>=2) buffer[1][len]=samples.getInt(1);
            //            ofs++;
            //std::cout << "0: " << samples.getInt(0) << ", 1: " << samples.getInt(1) << std::endl;
            len++;

            if (!samples.next()) {
                done=true;
                break;
            }
        }

        FLAC__stream_encoder_process(stream, input_, len);
        
    }

    FLAC__stream_encoder_finish(stream);
    FLAC__stream_encoder_delete(stream);
    return true;
}




// Flac decoder

struct DecodedFrame {
    void* buffer;
    size_t bytes;
};

struct DecoderInfo {
    DecoderInfo() {
        reader=0;
        totalSamples=0;
    }
    std::vector<DecodedFrame> buffers;
    size_t totalSamples;
    zzub::instream* reader;

};

//The address of the buffer to be filled is supplied, along with the number of bytes the
// buffer can hold. The callback may choose to supply less data and modify the byte count
// but must be careful not to overflow the buffer. The callback then returns a status code
// chosen from FLAC__StreamDecoderReadStatus.
static FLAC__StreamDecoderReadStatus flac_stream_decoder_read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data) {
    DecoderInfo* info = (DecoderInfo*)client_data;

    if (info->reader->position() >= info->reader->size()-1) return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;

    unsigned int bytesRead = info->reader->read(buffer, *bytes);
    if (bytesRead != *bytes) {
        *bytes = bytesRead;
    }
    return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

FLAC__StreamDecoderWriteStatus flac_stream_decoder_write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data) {
    DecoderInfo* info=(DecoderInfo*)client_data;
    
    // An array of pointers to decoded channels of data. Each pointer will point to an array of signed samples of length frame->header.blocksize. Currently, the channel order has no meaning except for stereo streams; in this case channel 0 is left and 1 is right.
    size_t numSamples=frame->header.blocksize;
    size_t channels=frame->header.channels;
    int bytesPerSample=(frame->header.bits_per_sample / 8);
    size_t bufferSize=bytesPerSample*numSamples*channels;

    void* vp;
    char* cp=new char[bufferSize];
    vp=cp;

    for (size_t i=0; i<numSamples; i++) {
        memcpy(cp, &buffer[0][i], bytesPerSample);
        cp+=bytesPerSample;
        if (channels==2) {
            memcpy(cp, &buffer[1][i], bytesPerSample);
            cp+=bytesPerSample;
        }
    }

    DecodedFrame df;
    df.bytes=bufferSize;
    df.buffer=vp;
    info->buffers.push_back(df);
    info->totalSamples+=numSamples;

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
    // FLAC__STREAM_DECODER_WRITE_STATUS_ABORT
}

void flac_stream_decoder_metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data) {
    //MessageBox(0, "we got meta", "", MB_OK);
}

void flac_stream_decoder_error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data) {
    //MessageBox(0, "we got error", "", MB_OK);
}

void decodeFLAC(zzub::instream* reader, zzub::player& player, int wave, int level) {
    FLAC__StreamDecoder* decoder = FLAC__stream_decoder_new();
    FLAC__stream_decoder_set_metadata_ignore_all(decoder);
    DecoderInfo decoder_info;
    decoder_info.reader = reader;
    FLAC__stream_decoder_init_stream(decoder,
                                     flac_stream_decoder_read_callback,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     flac_stream_decoder_write_callback,
                                     flac_stream_decoder_metadata_callback,
                                     flac_stream_decoder_error_callback,
                                     &decoder_info);
    FLAC__stream_decoder_process_until_end_of_stream(decoder);
    FLAC__stream_decoder_finish(decoder);
    int channels = FLAC__stream_decoder_get_channels(decoder);
    unsigned int bps = FLAC__stream_decoder_get_bits_per_sample(decoder);
    //unsigned int sample_rate = FLAC__stream_decoder_get_sample_rate(decoder);
    // allocate a level based on the stats retreived from the decoder stream
    zzub::wave_buffer_type waveFormat;
    switch (bps) {
    case 16:
        waveFormat = wave_buffer_type_si16;
        break;
    case 24:
        waveFormat = wave_buffer_type_si24;
        break;
    case 32:
        waveFormat = wave_buffer_type_si32;
        break;
    default:
        throw "not a supported bitsize";
    }
    player.wave_allocate_level(wave, level, decoder_info.totalSamples, channels, waveFormat);
    //bool result = info.allocate_level(level, decoder_info.totalSamples, waveFormat, channels==2);
    //assert(result); // lr: you don't want to let this one go unnoticed. either bail out or give visual cues.
    wave_info_ex& w = *player.back.wavetable.waves[wave];
    wave_level_ex& l = w.levels[level];
    char* targetBuf = (char*)l.samples;
    for (size_t i = 0; i < decoder_info.buffers.size(); i++) {
        DecodedFrame frame = decoder_info.buffers[i];
        memcpy(targetBuf, frame.buffer, frame.bytes);
        targetBuf += frame.bytes;
        delete[] (char*)frame.buffer; // discard buffer
    }
    // clean up
    FLAC__stream_decoder_delete(decoder);
}


}