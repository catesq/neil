
#include "libzzub/waveimport.h"
#include "libzzub/streams.h"
#include "libzzub/tools.h"


namespace zzub {
    /***

      waveimporter

  ***/
static sf_count_t instream_filelen (void *user_data) {
    zzub::instream* strm = (zzub::instream*)user_data ;
    return strm->size();
}

static sf_count_t instream_seek (sf_count_t offset, int whence, void *user_data) {
    zzub::instream* strm = (zzub::instream*)user_data ;
    strm->seek((long)offset, (int)whence);
    return strm->position();
}

static sf_count_t instream_read (void *ptr, sf_count_t count, void *user_data){
    zzub::instream* strm = (zzub::instream*)user_data ;
    return strm->read(ptr, count);
}

static sf_count_t instream_write (const void *ptr, sf_count_t count, void *user_data) {
    zzub::instream* strm = (zzub::instream*)user_data ;
    assert(false);
    return 0;
}

static sf_count_t instream_tell (void *user_data){
    zzub::instream* strm = (zzub::instream*)user_data ;
    return strm->position();
}

ssize_t mpg123_read_cb(void* user_data, void *buf, size_t count) {
    zzub::instream* strm = (zzub::instream*)user_data ;
    return strm->read(buf, count);
}

off_t mpg123_seek_cb(void* user_data, off_t offset, int whence) {
    zzub::instream* strm = (zzub::instream*)user_data ;
    strm->seek((long)offset, (int)whence);
    return strm->position();
}

bool import_mpg123::open(zzub::instream* strm) {
    mh = NULL;
    int err  = MPG123_OK;

    err = mpg123_init();
    if(err != MPG123_OK || (mh = mpg123_new(NULL, &err)) == NULL) {
        fprintf(stderr, "Basic setup goes wrong: %s", mpg123_plain_strerror(err));
        close();
        mh = 0;
        return false;
    }

    mpg123_param(mh, MPG123_VERBOSE, 2, 0); /* Brabble a bit about the parsing/decoding. */
    // mpg123_param(mh, MPG123_ADD_FLAGS, MPG123_SEEKBUFFER, 0.);
    // mpg123_param(mh, MPG123_ADD_FLAGS, MPG123_FORCE_FLOAT, 0);

    if(mpg123_replace_reader_handle(mh, mpg123_read_cb, mpg123_seek_cb, 0) != MPG123_OK)
    {
        fprintf(stderr, "mpg123 error: %s\n", mpg123_strerror(mh));
        close();
        mh = 0;
        return false;
    }

    if( mpg123_open_handle(mh, strm) != MPG123_OK
            /* Peek into track and get first output format. */
            || mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK )
    {
        fprintf( stderr, "Trouble with mpg123: %s\n", mpg123_strerror(mh) );
        close();
        mh = 0;
        return false;
    }

    if(encoding != MPG123_ENC_SIGNED_16 && encoding != MPG123_ENC_FLOAT_32)
    { /* Signed 16 is the default output format anyways; it would actually by only different if we forced it.
        So this check is here just for this explanation. */
        close();
        mh = 0;
        fprintf(stderr, "Bad encoding: 0x%x!\n", encoding);
        return false;
    }

    /* Ensure that this output format will not change (it could, when we allow it). */
    mpg123_format_none(mh);
    mpg123_format(mh, rate, channels, encoding);
    // mpg123_format_all(mh);

    printf("File is %i channels and %liHz.\n", channels, rate);
    printf("Encoding is 0x%x\n", encoding);

    mpg123_scan(mh);

    return true;
}

void import_mpg123::close() {
    assert(mh != 0);
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
}

int import_mpg123::get_wave_count() {
    if (mh != 0) return 1;
    return 0;
}

int import_mpg123::get_wave_level_count(int i) {
    assert(i == 0);
    if (mh != 0) return 1;
    return 0;
}

bool import_mpg123::get_wave_level_info(int i, int level, importwave_info& info) {
    assert(i == 0);
    assert(level == 0);
    if (i != 0 && level != 0) {
        return false;
    }
    info.channels = channels;

    switch (encoding) {
    case MPG123_ENC_SIGNED_16:
        info.format = wave_buffer_type_si16;
        break;
    case MPG123_ENC_FLOAT_32:
        info.format = wave_buffer_type_si16;
        break;
    default:
        return false;
    }

    info.sample_count = mpg123_length(mh);
    info.samples_per_second = rate;
    return true;
}
void import_mpg123::read_wave_level_samples(int i, int level, void* buffer) {
    assert(i == 0);
    assert(level == 0);
    importwave_info iwi;
    if (!get_wave_level_info(i, level, iwi)) {
        printf("error getting wave level info\n");
        return;
    }
    const int buffer_size = mpg123_outblock(mh);
    // const int buffer_size = 2048;

    size_t done = 0;
    int samples = 0;
    int err  = MPG123_OK;
    do
    {
//        int more_samples;
        unsigned char buf[buffer_size];
        err = mpg123_read( mh, buf, buffer_size, &done );
        CopySamples(&buf, buffer, done / sizeof(short), iwi.format, iwi.format, 1, 1, 0, samples);
        samples += done / sizeof(short);

    } while (err==MPG123_OK);

    if(err != MPG123_DONE)
        fprintf( stderr, "Warning: Decoding ended prematurely because: %s\n",
                 err == MPG123_ERR ? mpg123_strerror(mh) : mpg123_plain_strerror(err) );
}


import_sndfile::import_sndfile() {
    sf = 0;
    memset(&sfinfo, 0, sizeof(sfinfo));
}

bool import_sndfile::open(zzub::instream* strm) {
    memset(&sfinfo, 0, sizeof(sfinfo));
    SF_VIRTUAL_IO vio;
    vio.get_filelen = instream_filelen ;
    vio.seek = instream_seek;
    vio.read = instream_read;
    vio.write = instream_write;
    vio.tell = instream_tell;
    sf = sf_open_virtual(&vio, SFM_READ, &sfinfo, strm);

    if (!sf || !sfinfo.frames) {
        sf_close(sf);
        sf = 0;
        return false;
    }

    return true;
}

int import_sndfile::get_wave_count() {
    if (sf != 0) return 1;
    return 0;
}

int import_sndfile::get_wave_level_count(int i) {
    assert(i == 0);
    if (sf != 0) return 1;
    return 0;
}

bool import_sndfile::get_wave_level_info(int i, int level, importwave_info& info) {
    assert(i == 0);
    assert(level == 0);
    if (i != 0 && level != 0) {
        return false;
    }
    info.channels = sfinfo.channels;
    // All the type info below is set in such a way, that
    // during the convertion from float (soundfile library is set
    // to read in float) it would all convert to 16 bit PCM.
    // This was done due to simplicy, some machines only working
    // with 16 bit files and due to the fact that if it were not
    // for this notice you would have never noticed it did this.
    // With time, of course, this should be re-implemented in a
    // more pleasing manner.
    switch (sfinfo.format & SF_FORMAT_SUBMASK) {
    case SF_FORMAT_PCM_U8:
    case SF_FORMAT_PCM_S8:
    case SF_FORMAT_PCM_16:
        info.format = wave_buffer_type_si16;
        break;
    case SF_FORMAT_PCM_24:
        //info.format = wave_buffer_type_si24;
        info.format = wave_buffer_type_si16;
        break;
    case SF_FORMAT_PCM_32:
        //info.format = wave_buffer_type_si32;
        info.format = wave_buffer_type_si16;
        break;
    case SF_FORMAT_FLOAT:
        //info.format = wave_buffer_type_f32;
        info.format = wave_buffer_type_si16;
        break;
    default:
        return false;
    }
    info.sample_count = (int)sfinfo.frames;
    info.samples_per_second = sfinfo.samplerate;
    return true;
}

void import_sndfile::read_wave_level_samples(int i, int level, void* buffer) {
    assert(i == 0);
    assert(level == 0);
    importwave_info iwi;
    if (!get_wave_level_info(i, level, iwi)) {
        return;
    }
    const int stream_buffer_size = 2048;
    for (int i = 0; i < iwi.sample_count / stream_buffer_size; i++) {
        float f[2 * stream_buffer_size];
        sf_read_float(sf, f, iwi.channels * stream_buffer_size);
        CopySamples(&f, buffer, stream_buffer_size * iwi.channels,
                    wave_buffer_type_f32, iwi.format, 1, 1, 0,
                    i * iwi.channels * stream_buffer_size);
    }
    /* If the number of samples in the file is not an exact
       multiple of the stream_buffer_size then load the remaining
       samples.
    */
    int remainder = iwi.sample_count % stream_buffer_size;
    if (remainder) {
        int divided = iwi.sample_count / stream_buffer_size;
        float f[2 * stream_buffer_size];
        sf_read_float(sf, f, iwi.channels * remainder);
        CopySamples(&f, buffer, remainder * iwi.channels,
                    wave_buffer_type_f32, iwi.format, 1, 1, 0,
                    divided * iwi.channels * stream_buffer_size);
    }
}

void import_sndfile::close() {
    assert(sf != 0);
    sf_close(sf);
}


waveimporter::waveimporter() {
    imp = 0;
    plugins.push_back(new import_sndfile());
    // plugins.push_back(new import_mad());
    plugins.push_back(new import_mpg123());
}

waveimporter::~waveimporter() {
    for (size_t i = 0; i < plugins.size(); i++)
        delete plugins[i];
    plugins.clear();
}

importplugin* waveimporter::get_importer(std::string filename) {
    size_t dp = filename.find_last_of('.');
    if (dp == std::string::npos) return 0;
    std::string ext = filename.substr(dp + 1);
    transform(ext.begin(), ext.end(), ext.begin(), (int(*)(int))std::tolower);
    std::vector<importplugin*>::iterator i;
    for (i = plugins.begin(); i != plugins.end(); ++i) {
        std::vector<std::string> exts = (*i)->get_extensions();
        std::vector<std::string>::iterator j = find(exts.begin(), exts.end(), ext);
        if (j != exts.end()) return *i;
    }

    return 0;
}

bool waveimporter::open(std::string filename, zzub::instream* inf) {
    imp = get_importer(filename);
    if (!imp) return false;
    return imp->open(inf);
}

int waveimporter::get_wave_count() {
    assert(imp);
    return imp->get_wave_count();
}

int waveimporter::get_wave_level_count(int i) {
    return imp->get_wave_level_count(i);
}

bool waveimporter::get_wave_level_info(int i, int level, importwave_info& info) {
    return imp->get_wave_level_info(i, level, info);
}

void waveimporter::read_wave_level_samples(int i, int level, void* buffer) {
    imp->read_wave_level_samples(i, level, buffer);
}

void waveimporter::close() {
    imp->close();
    imp = 0;
}


}