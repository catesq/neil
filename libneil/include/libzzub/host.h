#pragma once

#include <vector>
#include "zzub/consts.h"
#include "zzub/zzub_typedefs.h"


namespace zzub {

struct wave_info;
struct wave_level;
enum sequence_type;
struct info;
struct outstream;
struct event_handler;
struct plugin;
struct host_info;
struct player;
struct song;

struct host {
    virtual const wave_info *get_wave(int index);
    virtual const wave_level *get_wave_level(int index, int level);
    virtual void message(const char *text);
    virtual void lock();
    virtual void unlock();
    virtual void set_swap_mode(bool free);
    virtual int get_write_position();
    virtual int get_play_position();
    virtual void set_play_position(int pos);
    virtual float **get_auxiliary_buffer();
    virtual void clear_auxiliary_buffer();
    virtual int get_next_free_wave_index();
    virtual bool allocate_wave(int index, int level, int samples, wave_buffer_type type, bool stereo, const char *name);
    virtual bool allocate_wave_direct(int index, int level, int samples, wave_buffer_type type, bool stereo, const char *name);
    virtual void midi_out(int time, unsigned int data);
    virtual int get_envelope_size(int wave, int envelope);
    virtual bool get_envelope_point(int wave, int envelope, int index, unsigned short &x, unsigned short &y, int &flags);
    virtual const wave_level *get_nearest_wave_level(int index, int note);
    virtual void set_track_count(int count);
    virtual int create_pattern(const char *name, int length);
    // virtual int get_pattern(int index);
    virtual char const *get_pattern_name(int _pattern);
    virtual int get_pattern_length(int _pattern);
    virtual int get_pattern_count();
    virtual void rename_pattern(char const *oldname, char const *newname);
    virtual void delete_pattern(int _pattern);
    virtual int get_pattern_data(int _pattern, int row, int group, int track, int field);
    virtual void set_pattern_data(int _pattern, int row, int group, int track, int field, int value);
    virtual zzub_sequence_t *create_sequence();
    virtual void delete_sequence(zzub_sequence_t *_sequence);
    virtual int get_sequence_data(int row);
    virtual void set_sequence_data(int row, int pattern);
    virtual sequence_type get_sequence_type(zzub_sequence_t *seq);
    virtual void _legacy_control_change(int group, int track, int param, int value);
    virtual int audio_driver_get_channel_count(bool input);
    virtual void audio_driver_write(int channel, float *samples, int buffersize);
    virtual void audio_driver_read(int channel, float *samples, int buffersize);
    virtual zzub_plugin_t *get_metaplugin();
    // virtual int get_metaplugin_by_index(int plugin_desc);
    virtual void control_change(zzub_plugin_t *_metaplugin, int group, int track, int param, int value, bool record, bool immediate);

    virtual zzub_sequence_t *get_playing_sequence(zzub_plugin_t *_metaplugin);
    virtual void *get_playing_row(zzub_sequence_t *_sequence, int group, int track);
    virtual int get_state_flags();
    virtual void set_state_flags(int state);
    virtual void set_event_handler(zzub_plugin_t *_metaplugin, event_handler *handler);
    virtual void remove_event_handler(zzub_plugin_t *_metaplugin, event_handler *handler);

    virtual void add_event_type_listener(zzub::event_type type, event_handler* handler);
    virtual void add_plugin_event_listener(zzub::event_type type, event_handler* handler);
    virtual void add_plugin_event_listener(int plugin_id, zzub::event_type type, event_handler* handler);

    virtual void remove_event_filter(event_handler* handler);

    virtual const char *get_wave_name(int index);
    virtual void set_internal_wave_name(zzub_plugin_t *_metaplugin, int index, const char *name);
    virtual void get_plugin_names(outstream *os);
    virtual zzub_plugin_t *get_metaplugin(const char *name);
    virtual info const *get_info(zzub_plugin_t *_metaplugin);
    virtual char const *get_name(zzub_plugin_t *_metaplugin);
    virtual bool get_input(int index, float *samples, int buffersize, bool stereo, float *extrabuffer);
    virtual bool get_osc_url(zzub_plugin_t *pmac, char *url);

    virtual const parameter* get_parameter_info(zzub_plugin_t* _metaplugin, int group, int param);

    // peerctrl extensions
    virtual int get_parameter(zzub_plugin_t *_metaplugin, int group, int track, int param);
    virtual void set_parameter(zzub_plugin_t *_metaplugin, int group, int track, int param, int value);
    virtual plugin *get_plugin(zzub_plugin_t *_metaplugin);
    virtual int get_plugin_id(zzub_plugin_t *_metaplugin);
    
    virtual plugin *get_plugin_by_id(int id);

    // hacked extensions
    virtual int get_song_begin();
    virtual void set_song_begin(int pos);
    virtual int get_song_end();
    virtual void set_song_end(int pos);
    virtual int get_song_begin_loop();
    virtual void set_song_begin_loop(int pos);
    virtual int get_song_end_loop();
    virtual void set_song_end_loop(int pos);
    virtual host_info *get_host_info();

    zzub::player *_player;
    // plugin_player is used for accessing plugins and is the
    // same as player except during initialization
    zzub::song *plugin_player;
    zzub_plugin_t *_plugin;

    host(zzub::player *, zzub_plugin_t *);
    virtual ~host();
    std::vector<std::vector<float> > aux_buffer;
    std::vector<std::vector<float> > feedback_buffer;
};

}