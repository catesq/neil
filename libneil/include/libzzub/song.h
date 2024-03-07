/*
  Copyright (C) 2003-2008 Anders Ervik <calvin@countzero.no>

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

#include "zzub/plugin.h"
#include "libzzub/graph.h"
#include "libzzub/wavetable.h"
#include "libzzub/master.h"
#include "libzzub/driver.h"
#include "libzzub/timer.h"
#include "libzzub/metaplugin.h"

using std::pair;
using std::string;
using std::vector;

namespace zzub {

struct connection;
struct info;
struct pluginlib;
struct operation;

struct _midiouts : zzub::outstream {
    std::vector<std::string> names;

    void clear() {
        names.clear();
    }
    virtual int write(void *buffer, int size) {
        char* pcbuf = (char*)buffer;
        names.push_back(std::string(pcbuf, pcbuf+size-1));
        return size;
    }
    virtual long position() { return 0; }
    virtual void seek(long, int) { }
};

// internal player states
enum player_state {
    player_state_playing,
    player_state_stopped,
    player_state_muted,
    player_state_released
};




struct metaplugin_proxy {
    metaplugin_proxy(player* _playr, int _id):_player(_playr),id(_id){}
    player* _player;
    unsigned int id;
};



//struct connection {
//    connection_type type;
//    void* connection_values;
//    vector<const parameter*> connection_parameters;

//    virtual ~connection() {};
//    virtual void process_events(zzub::song& player, const connection_descriptor& conn) = 0;
//    virtual bool work(zzub::song& player, const connection_descriptor& conn, int sample_count) = 0;

//protected:
//    // don't instantiate this class directly,
//    // use either audio_connection or events_connection or midi_connection or cv_connection
//    connection();
//};

struct keyjazz_note {
    int plugin_id;
    int timestamp;	// tick when note was played
    int group, track;
    int note;
    bool delay_off;		// set to true if a noteoff was sent on the same timestamp (tick)
};

struct midimapping {
    int plugin_id;
    int group, track, column;
    int channel, controller;

    bool operator == (const midimapping& mm) {
        return this == &mm;
    }
};

struct event_message {
    int plugin_id;
    event_handler* event;
    zzub_event_data data;
};

struct sequence_proxy {
    player* _player;
    int track;
    sequence_proxy(player* _playr, int _track):_player(_playr),track(_track){}
};

struct sequencer_track {
    int plugin_id;
    sequence_type type;
    int automation_group, automation_track, automation_column;
    int automation_mode;	// constant? linear? spline?
    vector<sequence_event> events;
    sequence_proxy* proxy;
};

// only valid in zzub::song
#define ASSERT_PLUGIN(plugin_id) assert(plugin_id >= 0 && plugin_id < plugins.size()); assert(plugins[plugin_id] != 0);

struct song {
    plugin_map graph;
    player_state state;
    vector<metaplugin*> plugins;
    vector<plugin_descriptor> work_order;
    vector<event_message> user_event_queue;
    unsigned int user_event_queue_read, user_event_queue_write;
    int enable_event_queue;
    vector<midimapping> midi_mappings;
    vector<sequencer_track> sequencer_tracks;
    wave_table wavetable;
    int song_begin, song_end, song_loop_begin, song_loop_end, song_loop_enabled;
    string song_comment;
    int seqstep;
    vector<keyjazz_note> keyjazz;
    int midi_plugin;

    song();
    virtual ~song() { }
    // plugin methods
    metaplugin& get_plugin(plugin_descriptor index);
    int get_plugin_count();
    plugin_descriptor get_plugin_descriptor(string name);
    int get_plugin_id(plugin_descriptor index);
    plugin_descriptor get_plugin_by_id(int id);
    void process_plugin_events(int plugin_id);
    void make_work_order();
    int get_plugin_parameter_track_row_bytesize(int plugin_id, int g, int t);
    void transfer_plugin_parameter_track_row(int plugin_id, int g, int t, const pattern& from_pattern, void* param_ptr, int row, bool copy_all);
    void transfer_plugin_parameter_row(int plugin_id, int g, const pattern& from_pattern, pattern& target_pattern, int from_row, int target_row, bool copy_all);
    void transfer_plugin_parameter_track_row(int plugin_id, int g, int t, const void* source, zzub::pattern& to_pattern, int row, bool copy_all);

    // connection:
    int plugin_get_input_connection_count(int plugin_id);
    int plugin_get_input_connection_plugin(int plugin_id, int index);
    connection_type plugin_get_input_connection_type(int plugin_id, int index);
    connection* plugin_get_input_connection(int plugin_id, int index);
    connection* plugin_get_input_connection(int plugin_id, int from_id, connection_type type);
    std::vector<connection*> plugin_get_input_connections(int plugin_id, int from_id);

    int plugin_get_input_connection_index(int to_id, int from_id, connection_type type);
    int plugin_get_output_connection_count(int to_id);
    connection* plugin_get_output_connection(int plugin_id, int index);
    connection* plugin_get_output_connection(int plugin_id, int from_id, connection_type type);
    int plugin_get_output_connection_index(int to_id, int from_id, connection_type type);
    int plugin_get_output_connection_plugin(int plugin_id, int index);
    connection_type plugin_get_output_connection_type(int plugin_id, int index);

    // pattern methods
    void create_pattern(pattern& p, int from_id, int rows);

    // pattern utility:

    void reset_plugin_parameter_group(pattern::group& group, const vector<const parameter*>& parameters);
    void reset_plugin_parameter_track(pattern::track& track, const vector<const parameter*>& parameters);
    void default_plugin_parameter_track(pattern::track& track, const vector<const parameter*>& parameters);
    void set_pattern_tracks(pattern& p, const vector<const parameter*>& parameters, int tracks, bool set_defaults);
    void set_pattern_length(int plugin_id, pattern& p, int rows);
    void add_pattern_connection_track(zzub::pattern& pattern, const vector<const parameter*>& parameters);

    int plugin_get_parameter_count(int plugin_id, int group, int track);
    int plugin_get_track_count(int plugin_id, int group);
    const parameter* plugin_get_parameter_info(int plugin_id, int group, int track, int column);
    int plugin_get_parameter(int plugin_id, int group, int track, int column);
    int plugin_get_parameter_direct(int plugin_id, int group, int track, int column);
    void plugin_set_parameter_direct(int plugin_id, int group, int track, int column, int value, bool record);
    zzub::info* create_dummy_info(int flags, string pluginUri, int attributes, int globalValues, int trackValues, parameter* params);
    void invoke_plugin_parameter_changes(int plugin_id);
    void invoke_plugin_parameter_changes(int plugin_id, int g);
    bool plugin_invoke_event(int plugin_id, zzub_event_data data, bool immediate = false);

    zzub::port* plugin_get_port(int plugin_id, int index);
    int plugin_get_port_count(int plugin_id);

    // plugin methods
    string plugin_describe_parameter(plugin_descriptor plugindesc, int group, int track, int column);
    string plugin_describe_value(plugin_descriptor plugindesc, int group, int column, int value);

    virtual bool plugin_update_keyjazz(int plugin_id, int note, int prev_note, int velocity, int& note_group, int& note_track, int& note_column, int& velocity_column) {
        assert(false);	// only use the derived mixer::plugin_update_keyjazz()
        return false;
    }
    virtual void set_play_position(int position) {
        assert(false);	// only use the derived mixer::set_play_position()
    }
    int sequencer_get_event_at(int track, unsigned long timestamp);

    // these are helpers called from operations:
    virtual void set_state(player_state newstate);
    void plugin_add_input(int to_id, int from_id, connection_type type);
    void plugin_delete_input(int to_id, int from_id, connection_type type);
private:
    // the input plugins are stored as out edges and vice versa
    // this is confusing and i'm not sure why it's done this way 
    // i see it as the plugin requesting audio from the plugins out edges to collect the plugins audio input
    // and suspect the out_edge=>input audio and in_edge=>output audio oddity is something to do with how make_work_order functions
    std::pair<zzub::out_edge_iterator, zzub::out_edge_iterator> get_plugin_input_edges(int plugin_id);
    std::pair<zzub::in_edge_iterator, zzub::in_edge_iterator> get_plugin_output_edges(int plugin_id);
    zzub::out_edge_iterator get_plugin_input_edge(int plugin_id, int index);
    zzub::in_edge_iterator get_plugin_output_edge(int plugin_id, int index);

    std::pair<connection*, int> plugin_get_input_connection_and_index(int plugin_id, int from_id, connection_type type);
    std::pair<connection*, int> plugin_get_output_connection_and_index(int plugin_id, int from_id, connection_type type);


};


struct mixer : song {
    bool is_recording_parameters;
    bool is_syncing_midi_transport;
    int song_position;								// current song position
    int work_position;								// total accumulation of samples processed
    int work_chunk_size;							// size of chunk in current buffer we're mixing
    int last_tick_work_position;					// at which workPos we last ticked
    int last_tick_position;							// at which song position we last ticked
    player_state last_tick_state;					// whether mixer state was playing or stopped last tick
    double work_tick_fracs;							// accumulated fractions of samples not processed
    std::vector<int> sequencer_indices;				// currently playing index in each track
    zzub::master_info master_info;
    master_plugin_info master_plugininfo;
    plugin_descriptor solo_plugin;
    vector<vector<float> > mix_buffer;
    float* inputBuffer[audiodriver::MAX_CHANNELS];
    float* outputBuffer[audiodriver::MAX_CHANNELS];

    zzub::timer timer;								// hires timer, for cpu-meter

    string load_error;
    string load_warning;

    mixer();

    // processing methods
    int generate_audio(int sample_count);
    void work_plugin(plugin_descriptor plugindesc, int sample_count);
    void process_sequencer_events(plugin_descriptor plugindesc);
    int determine_chunk_size(int sample_count, double& tick_fracs, int& next_tick_position);
    void process_sequencer_events();
    void process_keyjazz_noteoff_events();

    // midi
    void midi_event(unsigned short status, unsigned char data1, unsigned char data2);

    // plugin utility
    pattern create_play_note_pattern(int id, int note, int prevNote, int velocity, std::vector<keyjazz_note>& keyjazz);
    virtual bool plugin_update_keyjazz(int plugin_id, int note, int prev_note, int velocity, int& note_group, int& note_track, int& note_column, int& velocity_column);
    bool get_currently_playing_pattern(int plugin_id, int& pattern, int& row);
    bool get_currently_playing_pattern_row(int plugin_id, int pattern, int& row);

    void sequencer_update_play_pattern_positions();
    virtual void set_state(player_state newstate);
    virtual void set_play_position(int position);

};

};

