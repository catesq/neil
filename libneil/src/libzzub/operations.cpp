#include "libzzub/common.h"
#include <functional>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <iostream>
#include "libzzub/archive.h"
#include "libzzub/tools.h"


using std::cerr;
using std::endl;

namespace zzub {
struct find_note_column {
    bool operator()(const zzub::parameter* param) {
        return param->type == zzub::parameter_type_note;
    }
};

struct find_wave_column {
    bool operator()(const zzub::parameter* param) {
        return param->flags & zzub::parameter_flag_wavetable_index;
    }
};

struct find_velocity_column {
    bool operator()(const zzub::parameter* param) {
        return param->description != 0 && ( (strstr(param->description, "Velocity")) || (strstr(param->description, "Volume")) );
    }
};

bool get_note_info(const zzub::info* info, int& note_group, int& note_column) {
    std::vector<const zzub::parameter*>::const_iterator param;

    param = find_if(info->global_parameters.begin(), info->global_parameters.end(), find_note_column());
    if (param != info->global_parameters.end()) {
        note_group = 1;
        note_column = int(param - info->global_parameters.begin());
        return true;
    }

    param = find_if(info->track_parameters.begin(), info->track_parameters.end(), find_note_column());
    if (param != info->track_parameters.end()) {
        note_group = 2;
        note_column = int(param - info->track_parameters.begin());
        return true;
    }

    return false;
}

bool get_wave_info(const zzub::info* info, int note_group, int& wave_column) {
    std::vector<const zzub::parameter*>::const_iterator param;

    if(note_group == 1)
    {
        param = find_if(info->global_parameters.begin(), info->global_parameters.end(), find_wave_column());
        if (param != info->global_parameters.end()) {
            wave_column = int(param - info->global_parameters.begin());
            return true;
        }
    }

    param = find_if(info->track_parameters.begin(), info->track_parameters.end(), find_wave_column());
    if (param != info->track_parameters.end()) {
        wave_column = int(param - info->track_parameters.begin());
        return true;
    }

    return false;
}

bool get_velocity_info(const zzub::info* info, int note_group, int& velocity_column) {
    std::vector<const zzub::parameter*>::const_iterator param;

    if(note_group == 1)
    {
        param = find_if(info->global_parameters.begin(), info->global_parameters.end(), find_velocity_column());
        if (param != info->global_parameters.end()) {
            velocity_column = int(param - info->global_parameters.begin());
            return true;
        }
    }

    param = find_if(info->track_parameters.begin(), info->track_parameters.end(), find_velocity_column());
    if (param != info->track_parameters.end()) {
        velocity_column = int(param - info->track_parameters.begin());
        return true;
    }

    return false;
}



// ---------------------------------------------------------------------------
//
// op_state_change
//
// ---------------------------------------------------------------------------

op_state_change::op_state_change(zzub::player_state _state) {
    state = _state;
}

bool op_state_change::prepare(zzub::song& song) {
    event_data.type = event_type_player_state_changed;
    event_data.player_state_changed.player_state = state;
    return true;
}

bool op_state_change::operate(zzub::song& song) {
    song.set_state(state);
    return true;
}

void op_state_change::finish(zzub::song& song, bool send_events) {
    if (send_events) song.plugin_invoke_event(0, event_data, true);
}


// ---------------------------------------------------------------------------
//
// op_player_song_position
//
// ---------------------------------------------------------------------------

op_player_song_position::op_player_song_position(int _song_position) {
    song_position = _song_position;
}

bool op_player_song_position::prepare(zzub::song& song) {
    return true;
}

bool op_player_song_position::operate(zzub::song& song) {
    song.set_play_position(song_position);
    return true;
}

// ---------------------------------------------------------------------------
//
// op_plugin_create
//
// ---------------------------------------------------------------------------

op_plugin_create::op_plugin_create(zzub::player* _player, int _id, std::string _name, std::vector<char>& _bytes, const zzub::info* _loader, int _flags) {
    assert(_id != -1);
    assert(_name != "");
    assert(_loader);

    player = _player;
    id = _id;
    bytes = _bytes;
    name = _name;
    loader = _loader;
    flags = _flags;

    copy_flags.copy_graph = true;
    copy_flags.copy_work_order = true;
    copy_flags.copy_plugins = true;
}

bool op_plugin_create::prepare(zzub::song& song) {

    zzub::plugin* instance = loader->create_plugin();
    if (instance == 0) {
        return false;
    }

    assert(id != -1);

    if (song.plugins.size() <= (size_t)id) {
        song.plugins.resize(id + 1);
    }

    assert(song.plugins[id] == 0);
    song.plugins[id] = new metaplugin();

    plugin_descriptor descriptor = add_vertex(song.graph);
    song.graph[descriptor].id = id;

    metaplugin& plugin = song.get_plugin(descriptor);

    plugin.name = name;
    plugin.plugin = instance;
    plugin.descriptor = descriptor;
    plugin.info = loader;
    plugin.flags = loader->flags | this->flags;
    plugin.tracks = 0;
    plugin.midi_input_channel = 17;	// 17 = play if selected
    plugin.is_bypassed = false;
    plugin.is_muted = false;
    plugin.sequencer_state = sequencer_event_type_none;
    plugin.x = 0;
    plugin.y = 0;
    plugin.last_work_audio_result = false;
    plugin.last_work_midi_result = false;
    plugin.last_work_buffersize = 0;
    plugin.last_work_max_left = 0;
    plugin.last_work_max_right = 0;
    plugin.last_work_time = 0.0f;
    plugin.last_work_frame = 0;
    plugin.cpu_load = 0.0f;
    plugin.cpu_load_buffersize = 0;
    plugin.cpu_load_time = 0.0f;
    plugin.writemode_errors = 0;

    if (get_note_info(loader, plugin.note_group, plugin.note_column)) {
        if (!get_velocity_info(loader, plugin.note_group, plugin.velocity_column)) {
            plugin.velocity_column = -1;
        }
        if (!get_wave_info(loader, plugin.note_group, plugin.wave_column)) {
            plugin.wave_column = -1;
        }
    } else {
        plugin.note_group = -1;
        plugin.note_column = -1;
        plugin.velocity_column = -1;
        plugin.wave_column = -1;
    }


    plugin.initialized = false;
    plugin.work_buffer.resize(2);
    plugin.work_buffer[0].resize(buffer_size * 4);
    plugin.work_buffer[1].resize(buffer_size * 4);
    plugin.proxy = new metaplugin_proxy(player, id);
    plugin.callbacks = new host(player, plugin.proxy);
    plugin.callbacks->plugin_player = &song;
    plugin.plugin->_host = plugin.callbacks;
    plugin.plugin->_master_info = &player->front.master_info;

    // setting default attributes before init() makes some stereo wrapped machines work - instead of crashing in first attributesChanged
    if (instance->attributes)
        for (size_t i = 0; i < loader->attributes.size(); i++)
            instance->attributes[i] = loader->attributes[i]->value_default;

    // create state patterns before init(), in case plugins try to call
    // host::control_change in their init() (e.g Farbrasch V2)
    plugin.tracks = loader->min_tracks;
    // create state patterns with no-values
    song.create_pattern(plugin.state_write, id, 1);
    song.create_pattern(plugin.state_last, id, 1);
    song.create_pattern(plugin.state_automation, id, 1);

    // NOTE: some plugins' init() may call methods on the host to retreive info about other plugins.
    // we handle this by setting callbacks->plugin_player to the backbuffer song until the plugin
    // is swapped into the running graph
    if (bytes.size() > 0) {
        mem_archive arc;
        arc.get_outstream("")->write(&bytes.front(), (int)bytes.size());
        instance->init(&arc);
    } else {
        instance->init(0);
    }

    const char* plugin_stream_source = instance->get_stream_source();
    plugin.stream_source = plugin_stream_source ? plugin_stream_source : "";
    // add states for controller columns
    if (plugin.info->flags & zzub::plugin_flag_has_event_output) {
        plugin.state_write.groups.push_back(pattern::group());
        plugin.state_last.groups.push_back(pattern::group());
        plugin.state_automation.groups.push_back(pattern::group());
        // the controller group has one track
        plugin.state_write.groups.back().push_back(pattern::track());
        plugin.state_last.groups.back().push_back(pattern::track());
        plugin.state_automation.groups.back().push_back(pattern::track());
        for (size_t i = 0; i < plugin.info->controller_parameters.size(); i++) {
            // the controller group (track 0) has multiple columns
            plugin.state_write.groups.back().back().push_back(pattern::column());
            plugin.state_write.groups.back().back().back().push_back(plugin.info->controller_parameters[i]->value_none);
            plugin.state_last.groups.back().back().push_back(pattern::column());
            plugin.state_last.groups.back().back().back().push_back(plugin.info->controller_parameters[i]->value_none);
            plugin.state_automation.groups.back().back().push_back(pattern::column());
            plugin.state_automation.groups.back().back().back().push_back(plugin.info->controller_parameters[i]->value_none);
        }
    }
    // fill state pattern with default values and copy to live
    song.default_plugin_parameter_track(plugin.state_write.groups[1][0], loader->global_parameters);
    //song.transfer_plugin_parameter_track_row(id, 1, 0, plugin.state_write, (char*)plugin.plugin->global_values, 0, true);
    //char* track_ptr = (char*)plugin.plugin->track_values;
    //int track_size = song.get_plugin_parameter_track_row_bytesize(id, 2, 0);
    for (int j = 0; j < plugin.tracks; j++) {
        song.default_plugin_parameter_track(plugin.state_write.groups[2][j], loader->track_parameters);
        //song.transfer_plugin_parameter_track_row(id, 2, j, plugin.state_write, track_ptr, 0, true);
        //track_ptr += track_size;
    }
    instance->set_track_count(plugin.tracks);
    instance->attributes_changed();
    song.process_plugin_events(id);
    song.make_work_order();
    event_data.type = event_type_new_plugin;
    event_data.new_plugin.plugin = plugin.proxy;
    return true;
}

bool op_plugin_create::operate(zzub::song& song) {
    metaplugin& plugin = *song.plugins[id];
    plugin.callbacks->plugin_player = &song;
    plugin.initialized = true;
    plugin.plugin->created();
    return true;
}

void op_plugin_create::finish(zzub::song& song, bool send_events) {
    assert(id >= 0 && id <= song.plugins.size());
    // plugin was deleted before it was inserted in the graph
    if (song.plugins[id] == 0)
        return;
    if (send_events)
        song.plugin_invoke_event(0, event_data, true);
}

// ---------------------------------------------------------------------------
//
// op_plugin_delete
//
// ---------------------------------------------------------------------------

op_plugin_delete::op_plugin_delete(zzub::player* _player, int _id) {
    player = _player;
    id = _id;

    copy_flags.copy_graph = true;
    copy_flags.copy_work_order = true;
    copy_flags.copy_plugins = true;
    copy_flags.copy_plugins_deep = true;
    copy_flags.copy_sequencer_tracks = true;
}

bool op_plugin_delete::prepare(zzub::song& song) {

    metaplugin& mpl = *song.plugins[id];

    plugin_descriptor plugin = mpl.descriptor;
    assert(plugin != graph_traits<plugin_map>::null_vertex());

    // send delete notification now - before any changes occur in any back/front buffers
    event_data.type = event_type_pre_delete_plugin;
    event_data.delete_plugin.plugin = mpl.proxy;
    song.plugin_invoke_event(0, event_data, true);

    clear_vertex(plugin, song.graph);
    remove_vertex(plugin, song.graph);

    mpl.descriptor = graph_traits<plugin_map>::null_vertex();
    song.make_work_order();

    // adjust descriptors in plugins
    for (size_t i = 0; i < song.plugins.size(); i++) {
        if (song.plugins[i] != 0 && song.plugins[i]->descriptor != graph_traits<plugin_map>::null_vertex() && song.plugins[i]->descriptor > plugin)
            song.plugins[i]->descriptor--;
    }

    event_data.type = event_type_delete_plugin;
    event_data.delete_plugin.plugin = mpl.proxy;

    return true;
}

bool op_plugin_delete::operate(zzub::song& song) {

    plugin = song.plugins[id];
    song.plugins[id] = 0;

    // clear events targeted for this plugin:
    size_t read_pos = song.user_event_queue_read;
    while (read_pos != song.user_event_queue_write) {
        event_message& ev = song.user_event_queue[read_pos];
        if (ev.plugin_id == id) ev.event = 0;
        if (read_pos == song.user_event_queue.size() - 1)
            read_pos = 0; else
            read_pos++;
    }

    // remove currently playing keyjazz notes for this plugin
    for (size_t i = 0; i < song.keyjazz.size(); ) {
        if (song.keyjazz[i].plugin_id == id)
            song.keyjazz.erase(song.keyjazz.begin() + i); else
            i++;
    }

    // clear midi plugin if necessary
    if (song.midi_plugin == id) song.midi_plugin = -1;

    return true;
}

void op_plugin_delete::finish(zzub::song& song, bool send_events) {
    // the plugin is now assumed to be completely cleaned out of the graph and the swapping is done

    assert(plugin != 0);

    if (send_events) song.plugin_invoke_event(0, event_data, true);

    plugin->plugin->destroy();
    // int ptn = plugin->patterns.size();
    delete plugin->callbacks;
    delete plugin->proxy;
    delete plugin;
    plugin = 0;

}

// ---------------------------------------------------------------------------
//
// op_plugin_set_position
//
// ---------------------------------------------------------------------------

op_plugin_replace::op_plugin_replace(int _id, const metaplugin& _plugin) {
    id = _id;
    plugin = _plugin;
    copy_flags.copy_plugins = true;

    operation_copy_plugin_flags pluginflags;
    pluginflags.plugin_id = id;
    pluginflags.copy_plugin = true;
    copy_flags.plugin_flags.push_back(pluginflags);
}

bool op_plugin_replace::prepare(zzub::song& song) {
    event_data.type = event_type_plugin_changed;
    event_data.plugin_changed.plugin = song.plugins[id]->proxy;
    return true;
}

bool op_plugin_replace::operate(zzub::song& song) {

    
    if (song.plugins[id] == 0) return true;

    song.plugins[id]->name = plugin.name;
    song.plugins[id]->x = plugin.x;
    song.plugins[id]->y = plugin.y;
    song.plugins[id]->is_muted = plugin.is_muted;
    song.plugins[id]->is_bypassed = plugin.is_bypassed;
    song.plugins[id]->midi_input_channel = plugin.midi_input_channel;
    return true;
}

void op_plugin_replace::finish(zzub::song& song, bool send_events) {
    if (send_events) song.plugin_invoke_event(0, event_data, true);
}
// ---------------------------------------------------------------------------
//
// op_plugin_connect
//
// ---------------------------------------------------------------------------

op_plugin_connect::op_plugin_connect(int from_id, int to_id, zzub::connection_type type) {
    this->from_id = from_id;
    this->to_id = to_id;
    this->type = type;

    copy_flags.copy_graph = true;
    copy_flags.copy_work_order = true;
    copy_flags.copy_plugins = true;

    operation_copy_plugin_flags pluginflags;
    pluginflags.plugin_id = to_id;
    pluginflags.copy_plugin = true;
    pluginflags.copy_patterns = true;
    copy_flags.plugin_flags.push_back(pluginflags);
}

bool op_plugin_connect::prepare(zzub::song& song) {

    if (to_id == from_id) {
        cerr << "tried to connect " << to_id << " to itself" << endl;
        return false;
    }

    plugin_descriptor to_plugin = song.plugins[to_id]->descriptor;
    plugin_descriptor from_plugin = song.plugins[from_id]->descriptor;
    assert(to_plugin != graph_traits<plugin_map>::null_vertex());
    assert(from_plugin != graph_traits<plugin_map>::null_vertex());

    // check for duplicate connection
    if (song.plugin_get_input_connection_index(to_id, from_id, type) != -1) {
        cerr << "duplicate connection: " << song.plugin_get_input_connection_count(to_id) << endl;
        return false;
    }

    // check for existing connection in opposite direction
    if (song.plugin_get_input_connection_index(from_id, to_id, type) != -1) {
        cerr << "existing connection in opposite direction not allowed" << endl;
        return false;
    }

    // check if the plugins support requested flags
    int to_flags = song.plugins[to_id]->info->flags;
    int from_flags = song.plugins[from_id]->info->flags;
    bool to_has_audio = to_flags & plugin_flag_has_audio_input;
    bool from_has_audio = from_flags & plugin_flag_has_audio_output;
    bool to_has_midi = to_flags & plugin_flag_has_midi_input;
    bool from_has_midi = from_flags & plugin_flag_has_midi_output;
    //bool to_has_event = to_flags & plugin_flag_has_event_input;
    bool from_has_event = from_flags & plugin_flag_has_event_output;

    switch (type) {
    case connection_type_audio:
        if (!(to_has_audio && from_has_audio)) {
            cerr << "plugins dont support audio connection, " << song.plugins[from_id]->name << " -> " << song.plugins[to_id]->name << endl;
            return false;
        }
        break;
    case connection_type_midi:
        if (!(to_has_midi && from_has_midi)) {
            cerr << "plugins dont support midi connection" << song.plugins[from_id]->name << " -> " << song.plugins[to_id]->name << endl;
            return false;
        }
        break;
    case connection_type_event:
        if (!(from_has_event)) {
            cerr << "plugins dont support event connection" << song.plugins[from_id]->name << " -> " << song.plugins[to_id]->name << endl;
            return false;
        }
        break;
    case connection_type_cv:
    default:
        break;
    }

    metaplugin& to_mpl = *song.plugins[to_id];
    metaplugin& from_mpl = *song.plugins[from_id];

    // invoke pre-event so hacked plugins can lock the player
    event_data.type = event_type_pre_connect;
    event_data.connect_plugin.to_plugin = to_mpl.proxy;
    event_data.connect_plugin.from_plugin = from_mpl.proxy;
    event_data.connect_plugin.type = type;
    song.plugin_invoke_event(0, event_data, true);

    // if there are existing plugins with no_undo connected to to_plugin,
    // we need to disconnect them here, add the new input, and reconnect
    // afterwards because: otherwise the connection indices become invalid
    // when the no_undo plugin disconnects later

    // TODO: we want to remember connection volumes, patterns, event bindings etc
    typedef pair<int, connection_type> conninfo;
    vector<conninfo> no_undo_connections;
    for (int i = 0; i < song.plugin_get_input_connection_count(to_id); i++) {
        int cid = song.plugin_get_input_connection_plugin(to_id, i);
        int cflags = song.plugins[cid]->flags;
        if (cflags & zzub_plugin_flag_no_undo) {
            connection_type ctype = song.plugin_get_input_connection_type(to_id, i);
            song.plugin_delete_input(to_id, cid, ctype);
            no_undo_connections.push_back(conninfo(cid, ctype));
        }
    }

    // do the connecting we're actually supposed to do:
    song.plugin_add_input(to_id, from_id, type);

    // set initial connection states
    // int conn_count = song.plugin_get_input_connection_count(to_id);
    int conn_index = song.plugin_get_input_connection_index(to_id, from_id, type);
    connection* conn = song.plugin_get_input_connection(to_id, conn_index);

    for (size_t i = 0; i < values.size() && i < conn->connection_parameters.size(); i++) {
        to_mpl.state_write.groups[0].back()[i][0] = values[i];
    }

    switch (type) {
    case connection_type_midi:
        ((midi_connection*)conn)->device_name = midi_device;
        break;
    case connection_type_event:
        ((event_connection*)conn)->bindings = bindings;
        break;
    case connection_type_cv:
        ((cv_connection*)conn)->connectors = connectors;
        break;
    case connection_type_audio:
    default:
        break;
    }

    // reconnect no-undo plugins
    for (vector<conninfo>::iterator i = no_undo_connections.begin(); i != no_undo_connections.end(); ++i) {
        song.plugin_add_input(to_id, i->first, i->second);
    }

    from_name = from_mpl.name;

    event_data.type = event_type_connect;
    event_data.connect_plugin.to_plugin = to_mpl.proxy;
    event_data.connect_plugin.from_plugin = from_mpl.proxy;

    return true;
}

bool op_plugin_connect::operate(zzub::song& song) {
    song.plugins[to_id]->plugin->add_input(from_name.c_str(), type);
    return true;
}

void op_plugin_connect::finish(zzub::song& song, bool send_events) {
    if (send_events) song.plugin_invoke_event(0, event_data, true);

    event_data.type = event_type_post_connect;
    song.plugin_invoke_event(0, event_data, true);
}

// ---------------------------------------------------------------------------
//
// op_plugin_disconnect
//
// ---------------------------------------------------------------------------

op_plugin_disconnect::op_plugin_disconnect(int _from_id, int _to_id, zzub::connection_type _type) {
    from_id = _from_id;
    to_id = _to_id;
    type = _type;

    copy_flags.copy_graph = true;
    copy_flags.copy_work_order = true;
    copy_flags.copy_plugins = true;

    operation_copy_plugin_flags pluginflags;
    pluginflags.plugin_id = to_id;
    pluginflags.copy_plugin = true;
    pluginflags.copy_patterns = true;
    copy_flags.plugin_flags.push_back(pluginflags);

    conn = 0;
}

bool op_plugin_disconnect::prepare(zzub::song& song) {

    metaplugin& to_mpl = *song.plugins[to_id];
    metaplugin& from_mpl = *song.plugins[from_id];

    plugin_descriptor to_plugin = to_mpl.descriptor;
    plugin_descriptor from_plugin = from_mpl.descriptor;
    assert(to_plugin != graph_traits<plugin_map>::null_vertex());
    assert(from_plugin != graph_traits<plugin_map>::null_vertex());

    // invoke pre-event so hacked plugins can lock the player
    event_data.type = event_type_pre_disconnect;
    event_data.disconnect_plugin.to_plugin = to_mpl.proxy;
    event_data.disconnect_plugin.from_plugin = from_mpl.proxy;
    event_data.disconnect_plugin.type = type;
    song.plugin_invoke_event(0, event_data, true);

    from_name = from_mpl.name;
    int conn_index = song.plugin_get_input_connection_index(to_id, from_id, type);
    conn = song.plugin_get_input_connection(to_id, conn_index);

    song.plugin_delete_input(to_id, from_id, type);

    event_data.type = event_type_disconnect;
    event_data.disconnect_plugin.to_plugin = to_mpl.proxy;
    event_data.disconnect_plugin.from_plugin = from_mpl.proxy;

    return true;
}

bool op_plugin_disconnect::operate(zzub::song& song) {
    metaplugin& m = *song.plugins[to_id];
    m.plugin->delete_input(from_name.c_str(), type);
    return true;
}

void op_plugin_disconnect::finish(zzub::song& song, bool send_events) {
    if (send_events) song.plugin_invoke_event(0, event_data, true);
    assert(conn != 0);
    delete conn;
    conn = 0;
}

// ---------------------------------------------------------------------------
//
// op_plugin_set_midi_connection_device
//
// ---------------------------------------------------------------------------

op_plugin_set_midi_connection_device::op_plugin_set_midi_connection_device(int _to_id, int _from_id, std::string _name) {
    to_id = _to_id;
    from_id = _from_id;
    device = _name;

    copy_flags.copy_graph = true;
    copy_flags.copy_plugins = true;
}

bool op_plugin_set_midi_connection_device::prepare(zzub::song& song) {
    return true;
}

bool op_plugin_set_midi_connection_device::operate(zzub::song& song) {
    int conn_index = song.plugin_get_input_connection_index(to_id, from_id, connection_type_midi);
    // since we are in operate(), this operation could be a part of a compound operation
    // where the connection has been removed and is no longer valid. ultimately, the connection
    // operations should work on swapped-out connection copies in the future:
    if (conn_index == -1) return true;
    assert(conn_index != -1);

    midi_connection* conn = (midi_connection*)song.plugin_get_input_connection(to_id, conn_index);
    conn->device_name = device;
    return true;
}

void op_plugin_set_midi_connection_device::finish(zzub::song& song, bool send_events) {
}


// ---------------------------------------------------------------------------
//
// op_plugin_add_event_connection_binding
//
// ---------------------------------------------------------------------------

op_plugin_add_event_connection_binding::op_plugin_add_event_connection_binding(int _to_id, int _from_id, event_connection_binding _binding) {
    to_id = _to_id;
    from_id = _from_id;
    binding = _binding;
    copy_flags.copy_graph = true;
    copy_flags.copy_plugins = true;
}

bool op_plugin_add_event_connection_binding::prepare(zzub::song& song) {
    return true;
}

bool op_plugin_add_event_connection_binding::operate(zzub::song& song) {
    int conn_index = song.plugin_get_input_connection_index(to_id, from_id, connection_type_event);
    assert(conn_index != -1);
    event_connection* conn = (event_connection*)song.plugin_get_input_connection(to_id, conn_index);
    conn->bindings.push_back(binding);
    return true;
}

void op_plugin_add_event_connection_binding::finish(zzub::song& song, bool send_events) {
}


// ---------------------------------------------------------------------------
//
// op_plugin_remove_event_connection_binding
//
// ---------------------------------------------------------------------------

op_plugin_remove_event_connection_binding::op_plugin_remove_event_connection_binding(int _to_id, int _from_id, int _index) {
    to_id = _to_id;
    from_id = _from_id;
    index = _index;
    copy_flags.copy_graph = true;
    copy_flags.copy_plugins = true;
}

bool op_plugin_remove_event_connection_binding::prepare(zzub::song& song) {
    return true;
}

bool op_plugin_remove_event_connection_binding::operate(zzub::song& song) {
    int conn_index = song.plugin_get_input_connection_index(to_id, from_id, connection_type_event);
    if (conn_index == -1) return true;	// plugin was deleted
    assert(conn_index != -1);
    event_connection* conn = (event_connection*)song.plugin_get_input_connection(to_id, conn_index);
    if (index == -1) index = conn->bindings.size() -1;
    conn->bindings.erase(conn->bindings.begin() + index);
    return true;
}

void op_plugin_remove_event_connection_binding::finish(zzub::song& song, bool send_events) {
}

// ---------------------------------------------------------------------------
//
// op_plugin_add_cv_port_link
//
// ---------------------------------------------------------------------------


op_plugin_add_cv_connector::op_plugin_add_cv_connector(int to_id, int from_id, const cv_connector& connector) :
    from_id(from_id),
    to_id(to_id),
    connector(connector),
    plugin_connect_op(from_id, to_id, connection_type_cv) {
        copy_flags = plugin_connect_op.copy_flags;
}

bool op_plugin_add_cv_connector::prepare(zzub::song& song) {
    if(song.plugin_get_input_connection(to_id, from_id, connection_type_cv)) {
        this->do_plugin_connect = false;
    } else {
        this->do_plugin_connect = this->plugin_connect_op.prepare(song);

        if(!this->do_plugin_connect)
            return false;
    }

    return true;
}

bool op_plugin_add_cv_connector::operate(zzub::song& song) {
    if(this->do_plugin_connect) {
        plugin_connect_op.operate(song);
    }

    auto conn = song.plugin_get_input_connection(to_id, from_id, connection_type_cv);

    if(!conn)    
        return false;
    
    static_cast<cv_connection*>(conn)->add_connector(
        connector,
        *song.plugins[from_id],
        *song.plugins[to_id]
    );

    song.plugins[to_id]->plugin->connect_ports(connector);
    song.plugins[from_id]->plugin->connect_ports(connector);
    
    return true;
}

void op_plugin_add_cv_connector::finish(zzub::song& song, bool send_events) {
    if(this->do_plugin_connect)
        plugin_connect_op.finish(song, send_events);
}

// ---------------------------------------------------------------------------
//
// op_plugin_remove_cv_port_link
//
// ---------------------------------------------------------------------------



op_plugin_remove_cv_connector::op_plugin_remove_cv_connector(int to_id, int from_id, const cv_connector& connector) :
    to_id(to_id),
    from_id(from_id),
    connector(connector),
    plugin_disconnect_op(to_id, from_id, connection_type_cv) {
}


bool op_plugin_remove_cv_connector::prepare(zzub::song& song) {
    auto conn = song.plugin_get_input_connection(to_id, from_id, connection_type_cv);

    if (!conn) {
        return false;
    }

    auto cv_conn = static_cast<cv_connection*>(conn);

    // remove the cv_connection from the connection graph 
    // if this is the last cv connector between the two plugins
    if(cv_conn->has_connector(connector) && cv_conn->get_connector_count() == 1) {
        do_plugin_disconnect = true;
        plugin_disconnect_op.prepare(song);
    }

    return true;
}


bool op_plugin_remove_cv_connector::operate(zzub::song& song) {
    auto conn = (cv_connection*) song.plugin_get_input_connection(to_id, from_id, connection_type_cv);

    if (!conn) 
        return true;	// plugin already deleted somehow

    for(auto it = conn->connectors.begin(); it != conn->connectors.end(); ) {
        if (*it == connector) {
            it = conn->connectors.erase(it);
        } else {
            ++it;
        }
    }

    song.plugins[to_id]->plugin->disconnect_ports(connector);
    song.plugins[from_id]->plugin->disconnect_ports(connector);

    if(do_plugin_disconnect)
        plugin_disconnect_op.operate(song);

    return true;
}


void op_plugin_remove_cv_connector::finish(zzub::song& song, bool send_events) {
    if(do_plugin_disconnect)
        plugin_disconnect_op.finish(song, send_events);
}



// ---------------------------------------------------------------------------
//
// op_plugin_edit_cv_port_link
//
// ---------------------------------------------------------------------------

op_plugin_edit_cv_connector::op_plugin_edit_cv_connector(
    int to_id, 
    int from_id, 
    const cv_connector& old_connector, 
    const cv_connector& new_connector
) :
    from_id(from_id),
    to_id(to_id),
    old_connector(old_connector),
    new_connector(new_connector) {
}


bool op_plugin_edit_cv_connector::prepare(zzub::song& song) {    
    return true;
}


bool op_plugin_edit_cv_connector::operate(zzub::song& song) {
    auto connection = (cv_connection*) song.plugin_get_input_connection(to_id, from_id, connection_type_cv);

    if (!connection) {
        return false;
    }

    song.plugins[to_id]->plugin->disconnect_ports(old_connector);
    song.plugins[from_id]->plugin->disconnect_ports(old_connector);

    connection->update_connector(
        old_connector, 
        new_connector,
        *song.plugins[from_id],
        *song.plugins[to_id]
    );

    song.plugins[to_id]->plugin->disconnect_ports(old_connector);
    song.plugins[from_id]->plugin->disconnect_ports(old_connector);

    return true;
}


void op_plugin_edit_cv_connector::finish(zzub::song& song, bool send_events) {
}




// ---------------------------------------------------------------------------
//
// op_plugin_sequencer_set_tracks
//
// ---------------------------------------------------------------------------

op_plugin_set_track_count::op_plugin_set_track_count(int _id, int _tracks) {
    id = _id;
    tracks = _tracks;

    copy_flags.copy_graph = true;
    copy_flags.copy_work_order = true;
    copy_flags.copy_plugins = true;

    operation_copy_plugin_flags pluginflags;
    pluginflags.plugin_id = id;
    pluginflags.copy_plugin = true;
    pluginflags.copy_patterns = true;
    copy_flags.plugin_flags.push_back(pluginflags);
}

bool op_plugin_set_track_count::prepare(zzub::song& song) {

    assert(song.plugins[id]);

    metaplugin& m = *song.plugins[id];

    assert(tracks >= m.info->min_tracks && tracks <= m.info->max_tracks);

    // invoke pre-event so hacked plugins can lock the player
    event_data.type = event_type_pre_set_tracks;
    event_data.set_tracks.plugin = m.proxy;
    song.plugin_invoke_event(0, event_data, true);

    for (size_t i = 0; i < m.patterns.size(); i++) {
        song.set_pattern_tracks(*m.patterns[i], m.info->track_parameters, tracks, false);
    }

    song.set_pattern_tracks(m.state_write, m.info->track_parameters, tracks, true);
    song.set_pattern_tracks(m.state_last, m.info->track_parameters, tracks, false);
    song.set_pattern_tracks(m.state_automation, m.info->track_parameters, tracks, false);

    m.tracks = tracks;

    event_data.type = event_type_set_tracks;
    event_data.set_tracks.plugin = m.proxy;

    return true;
}

bool op_plugin_set_track_count::operate(zzub::song& song) {
    assert(id >= 0 && id <= song.plugins.size());
    assert(song.plugins[id] != 0);

    metaplugin& m = *song.plugins[id];

    // check if plugin was already deleted
    if (m.descriptor == graph_traits<plugin_map>::null_vertex()) return false;

    char* track_ptr = (char*)m.plugin->track_values;
    int track_size = song.get_plugin_parameter_track_row_bytesize(id, 2, 0);
    for (int i = 0; i < m.tracks; i++) {
        song.transfer_plugin_parameter_track_row(id, 2, i, m.state_write, track_ptr, 0, true);
        track_ptr += track_size;
    }
    m.plugin->set_track_count(tracks);
    return true;
}

void op_plugin_set_track_count::finish(zzub::song& song, bool send_events) {
    if (send_events) song.plugin_invoke_event(0, event_data, true);

    event_data.type = event_type_post_set_tracks;
    song.plugin_invoke_event(0, event_data, true);
}

// ---------------------------------------------------------------------------
//
// op_plugin_set_parameters_and_tick
//
// ---------------------------------------------------------------------------

op_plugin_set_parameters_and_tick::op_plugin_set_parameters_and_tick(int _id, zzub::pattern& _pattern, int _row, bool _no_process) {
    id = _id;
    pattern = _pattern;
    row = _row;
    no_process = _no_process;
    record = false;
}

bool op_plugin_set_parameters_and_tick::prepare(zzub::song& song) {
    assert(song.plugins[id] != 0);
    metaplugin& m = *song.plugins[id];
    // write to backbuffer so we can read them later
    song.transfer_plugin_parameter_row(id, 0, pattern, m.state_write, row, 0, false);
    song.transfer_plugin_parameter_row(id, 1, pattern, m.state_write, row, 0, false);
    song.transfer_plugin_parameter_row(id, 2, pattern, m.state_write, row, 0, false);
    return true;
}

bool op_plugin_set_parameters_and_tick::operate(zzub::song& song) {
    // TODO: we could move all this into prepare and add a copy-plugin-flag instead
    // - the target plugin could have changed before this point, such having as added/removed a connection
    // - should make the boundary-check inside transfer_plugin_parameter_row redundant
    assert(song.plugins[id] != 0);

    metaplugin& m = *song.plugins[id];

    // check if the plugin was deleted
    if (m.descriptor == graph_traits<plugin_map>::null_vertex()) return true;

    m.sequencer_state = sequencer_event_type_none;

    song.transfer_plugin_parameter_row(id, 0, pattern, m.state_write, row, 0, false);
    song.transfer_plugin_parameter_row(id, 1, pattern, m.state_write, row, 0, false);
    song.transfer_plugin_parameter_row(id, 2, pattern, m.state_write, row, 0, false);

    if (record) {
        song.transfer_plugin_parameter_row(id, 0, pattern, m.state_automation, row, 0, false);
        song.transfer_plugin_parameter_row(id, 1, pattern, m.state_automation, row, 0, false);
        song.transfer_plugin_parameter_row(id, 2, pattern, m.state_automation, row, 0, false);
    }

    if (!no_process) song.process_plugin_events(id);
    return true;
}

// ---------------------------------------------------------------------------
//
// op_plugin_set_parameter
//
// ---------------------------------------------------------------------------

op_plugin_set_parameter::op_plugin_set_parameter(int _id, int _group, int _track, int _column, int _value, bool _record) {
    id = _id;
    group = _group;
    track = _track;
    column = _column;
    value = _value;
    record = _record;
    copy_flags.copy_plugins = true;

    operation_copy_plugin_flags pluginflags;
    pluginflags.plugin_id = id;
    pluginflags.copy_plugin = true;
    copy_flags.plugin_flags.push_back(pluginflags);
}

bool op_plugin_set_parameter::prepare(zzub::song& song) {
    assert(song.plugins[id] != 0);
    metaplugin& m = *song.plugins[id];
    // write to backbuffer so we can read them later
    m.state_write.groups[group][track][column][0] = value;
    if (record) m.state_automation.groups[group][track][column][0] = value;
    return true;
}

bool op_plugin_set_parameter::operate(zzub::song& song) {
    assert(song.plugins[id] != 0);

    metaplugin& m = *song.plugins[id];

    // check if the plugin was deleted
    if (m.descriptor == graph_traits<plugin_map>::null_vertex()) return true;

    m.sequencer_state = sequencer_event_type_none;
    //m.state_write.groups[group][track][column][0] = value;
    //if (record) m.state_automation.groups[group][track][column][0] = value;
    return true;
}

// ---------------------------------------------------------------------------
//
// op_plugin_process_events
//
// ---------------------------------------------------------------------------

op_plugin_process_events::op_plugin_process_events(int _id) {
    id = _id;
}

bool op_plugin_process_events::prepare(zzub::song& song) {
    return true;
}

bool op_plugin_process_events::operate(zzub::song& song) {
    song.process_plugin_events(id);
    return true;
}

// ---------------------------------------------------------------------------
//
// op_plugin_play_note
//
// ---------------------------------------------------------------------------

op_plugin_play_note::op_plugin_play_note(int _id, int _note, int _prev_note, int _velocity) {
    id = _id;
    note = _note;
    prev_note = _prev_note;
    velocity = _velocity;
    copy_flags.copy_plugins = true;
}

bool op_plugin_play_note::prepare(zzub::song& song) {
    return true;
}

bool op_plugin_play_note::operate(zzub::song& song) {
    assert(song.plugins[id] != 0);

    metaplugin& m = *song.plugins[id];

    // check if the plugin was deleted
    if (m.descriptor == graph_traits<plugin_map>::null_vertex()) return true;

    // find note_group, track, column and velocity_group, track and column based on keyjazz-struct
    int note_group = -1, note_track = -1, note_column = -1;
    int velocity_column = -1;
    song.plugin_update_keyjazz(id, note, prev_note, velocity, note_group, note_track, note_column, velocity_column);

    if (note_group != -1) {
        m.sequencer_state = sequencer_event_type_none;
        song.plugin_set_parameter_direct(id, note_group, note_track, note_column, note, true);
        if (velocity_column != -1 && velocity != 0)
            song.plugin_set_parameter_direct(id, note_group, note_track, velocity_column, velocity, true);

        song.process_plugin_events(id);
    }

    return true;
}

// ---------------------------------------------------------------------------
//
// op_plugin_set_event_handlers
//
// ---------------------------------------------------------------------------

op_plugin_set_event_handlers::op_plugin_set_event_handlers(std::string _name, std::vector<event_handler*>& _handlers) {
    name = _name;
    handlers = _handlers;
}

bool op_plugin_set_event_handlers::prepare(zzub::song& song) {
    event_data.type = -1;
    return true;
}

bool op_plugin_set_event_handlers::operate(zzub::song& song) {
    plugin_descriptor plugin = song.get_plugin_descriptor(name);
    assert(plugin != graph_traits<plugin_map>::null_vertex());

    song.get_plugin(plugin).event_handlers.swap(handlers);
    return true;
}



// ---------------------------------------------------------------------------
//
// op_plugin_set_event_handlers
//
// ---------------------------------------------------------------------------

op_plugin_set_stream_source::op_plugin_set_stream_source(int _id, std::string _data_url) {
    id = _id;
    data_url = _data_url;

    copy_flags.copy_plugins = true;

    operation_copy_plugin_flags pluginflags;
    pluginflags.plugin_id = id;
    pluginflags.copy_plugin = true;
    copy_flags.plugin_flags.push_back(pluginflags);

}

bool op_plugin_set_stream_source::prepare(zzub::song& song) {
    song.plugins[id]->stream_source = data_url;
    return true;
}

bool op_plugin_set_stream_source::operate(zzub::song& song) {
    assert(id >= 0);
    assert(song.plugins[id]);

    metaplugin& m = *song.plugins[id];
    m.plugin->set_stream_source(data_url.c_str());

    return true;
}


void op_plugin_set_stream_source::finish(zzub::song& song, bool send_events) {
    event_data.type = -1;
}

// ---------------------------------------------------------------------------
//
// op_pattern_edit
//
// ---------------------------------------------------------------------------

op_pattern_edit::op_pattern_edit(int _id, int _index, int _group, int _track, int _column, int _row, int _value) {
    id = _id;
    index = _index;

    group = _group;
    track = _track;
    column = _column;
    row = _row;
    value = _value;

    copy_flags.copy_plugins = true;	// TODO: read-only!!
}

bool op_pattern_edit::prepare(zzub::song& song) {

    assert(id < song.plugins.size());
    assert(song.plugins[id] != 0);
    assert(index >= 0 && (size_t)index < song.plugins[id]->patterns.size());
    assert(group >= 0 && group < song.plugins[id]->patterns[index]->groups.size());
    assert(track >= 0 && track < song.plugins[id]->patterns[index]->groups[group].size());
    assert(column >= 0 && column < song.plugins[id]->patterns[index]->groups[group][track].size());
    assert(row >= 0 && row < song.plugins[id]->patterns[index]->groups[group][track][column].size());

    const zzub::parameter* param = song.plugin_get_parameter_info(id, group, track, column);
    assert((value >= param->value_min && value <= param->value_max) || value == param->value_none  || (param->type == zzub::parameter_type_note && value == zzub::note_value_off));

    song.plugins[id]->patterns[index]->groups[group][track][column][row] = value;

    event_data.type = event_type_edit_pattern;
    event_data.edit_pattern.plugin = song.plugins[id]->proxy;
    event_data.edit_pattern.index = index;
    event_data.edit_pattern.group = group;
    event_data.edit_pattern.track = track;
    event_data.edit_pattern.column = column;
    event_data.edit_pattern.row = row;
    event_data.edit_pattern.value = value;

    return true;
}

bool op_pattern_edit::operate(zzub::song& song) {

    // TODO: may crash here if pattern/machine was deleted and edited in the same operation?
    song.plugins[id]->patterns[index]->groups[group][track][column][row] = value;

    return true;
}

void op_pattern_edit::finish(zzub::song& song, bool send_events) {
    if (send_events) song.plugin_invoke_event(0, event_data, true);
}

// ---------------------------------------------------------------------------
//
// op_pattern_insert
//
// ---------------------------------------------------------------------------

op_pattern_insert::op_pattern_insert(int _id, int _index, zzub::pattern _pattern) {
    id = _id;
    index = _index;
    pattern = _pattern;
    copy_flags.copy_graph = true;
    copy_flags.copy_plugins = true;

    operation_copy_plugin_flags pluginflags;
    pluginflags.plugin_id = id;
    pluginflags.copy_plugin = true;
    pluginflags.copy_patterns = true;
    copy_flags.plugin_flags.push_back(pluginflags);
}

bool op_pattern_insert::prepare(zzub::song& song) {
    metaplugin& m = *song.plugins[id];

    if (index == -1)
        m.patterns.push_back(new zzub::pattern(pattern));
    else
        m.patterns.insert(m.patterns.begin() + index, new zzub::pattern(pattern));

    event_data.type = event_type_new_pattern;
    event_data.new_pattern.plugin = m.proxy;
    event_data.new_pattern.index = m.patterns.size() - 1;
    return true;
}

bool op_pattern_insert::operate(zzub::song& song) {
    return true;
}

void op_pattern_insert::finish(zzub::song& song, bool send_events) {
    if (send_events)
        song.plugin_invoke_event(0, event_data, true);
}

// ---------------------------------------------------------------------------
//
// op_pattern_remove
//
// ---------------------------------------------------------------------------

op_pattern_remove::op_pattern_remove(int _id, int _index) {
    id = _id;
    index = _index;
    copy_flags.copy_graph = true;
    copy_flags.copy_plugins = true;
    copy_flags.copy_sequencer_tracks = true;

    operation_copy_plugin_flags pluginflags;
    pluginflags.plugin_id = _id;
    pluginflags.copy_plugin = true;
    pluginflags.copy_patterns = true;
    copy_flags.plugin_flags.push_back(pluginflags);
}

bool op_pattern_remove::prepare(zzub::song& song) {
    assert(song.plugins[id] != 0 && id >= 0 && id < song.plugins.size());

    metaplugin& m = *song.plugins[id];

    if (index == -1) index = m.patterns.size() - 1;

    assert(index >= 0 && (size_t)index < m.patterns.size());

    event_data.type = event_type_pre_delete_pattern;
    event_data.delete_pattern.plugin = m.proxy;
    event_data.delete_pattern.index = index;
    song.plugin_invoke_event(0, event_data, true);

    event_data.type = event_type_delete_pattern;

    // remove pattern from pattern list
    delete m.patterns[index];
    m.patterns.erase(m.patterns.begin() + index);

    // adjust pattern indices in the sequencer

    for (size_t i = 0; i < song.sequencer_tracks.size(); i++) {
        if (song.sequencer_tracks[i].plugin_id == id) {
            for (size_t j = 0; j < song.sequencer_tracks[i].events.size(); j++) {
                sequence_event& ev = song.sequencer_tracks[i].events[j];
                if (ev.pattern_event.value >= 0x10) {
                    int pattern = ev.pattern_event.value - 0x10;
                    assert(pattern != index);	// you should have deleted these already
                    if (index < pattern) {
                        ev.pattern_event.value--;
                    }
                }
            }
        }
    }

    return true;
}

bool op_pattern_remove::operate(zzub::song& song) {
    return true;
}

void op_pattern_remove::finish(zzub::song& song, bool send_events) {
    if (send_events) song.plugin_invoke_event(0, event_data, true);
}

// ---------------------------------------------------------------------------
//
// op_pattern_move
//
// ---------------------------------------------------------------------------

op_pattern_move::op_pattern_move(int _id, int _index, int _newindex) {
    id = _id;
    index = _index;
    newindex = _newindex;
    copy_flags.copy_graph = true;
    copy_flags.copy_plugins = true;
    copy_flags.copy_sequencer_tracks = true;

    operation_copy_plugin_flags pluginflags;
    pluginflags.plugin_id = id;
    pluginflags.copy_plugin = true;
    copy_flags.plugin_flags.push_back(pluginflags);
}

bool op_pattern_move::prepare(zzub::song& song) {
    metaplugin& m = *song.plugins[id];

    plugin_descriptor plugin = m.descriptor;
    assert(plugin != graph_traits<plugin_map>::null_vertex());

    std::vector<zzub::pattern*>& patterns = m.patterns;

    if (index == -1) index = patterns.size() - 1;

    assert(index >= 0 && (size_t)index < patterns.size());
    assert(newindex >= 0 && (size_t)newindex < patterns.size());

    zzub::pattern* patterncopy = patterns[index];
    patterns.erase(patterns.begin() + index);
    if (index < newindex)
        patterns.insert(patterns.begin() + newindex - 1, patterncopy); else
        patterns.insert(patterns.begin() + newindex, patterncopy);

    // update sequencer events
    for (size_t i = 0; i < song.sequencer_tracks.size(); i++) {
        if (song.sequencer_tracks[i].plugin_id == id) {
            for (size_t j = 0; j < song.sequencer_tracks[i].events.size(); j++) {
                sequence_event& ev = song.sequencer_tracks[i].events[j];
                if (ev.pattern_event.value >= 0x10) {
                    int pattern = ev.pattern_event.value - 0x10;
                    if (pattern == index) {
                        ev.pattern_event.value = newindex;
                    } else
                        if (newindex < index && pattern >= newindex && pattern < index) {
                            ev.pattern_event.value++;
                        } else
                            if (newindex > index && pattern > index && pattern <= newindex) {
                                ev.pattern_event.value--;
                            }
                }
            }
        }
    }

    event_data.type = event_type_plugin_changed;
    event_data.plugin_changed.plugin = m.proxy;
    return true;
}

bool op_pattern_move::operate(zzub::song& song) {
    return true;
}

void op_pattern_move::finish(zzub::song& song, bool send_events) {
    if (send_events) song.plugin_invoke_event(0, event_data, true);
}

// ---------------------------------------------------------------------------
//
// op_pattern_replace
//
// ---------------------------------------------------------------------------

op_pattern_replace::op_pattern_replace(int _id, int _index, const zzub::pattern& _pattern) {
    id = _id;
    index = _index;
    pattern = _pattern;

    copy_flags.copy_graph = true;
    copy_flags.copy_plugins = true;

    operation_copy_plugin_flags pluginflags;
    pluginflags.plugin_id = id;
    pluginflags.copy_plugin = true;
    pluginflags.copy_patterns = true;
    copy_flags.plugin_flags.push_back(pluginflags);

}

bool op_pattern_replace::prepare(zzub::song& song) {
    assert(id < song.plugins.size());
    assert(song.plugins[id] != 0);

    metaplugin& m = *song.plugins[id];

    delete m.patterns[index];
    m.patterns[index] = new zzub::pattern(pattern);

    event_data.type = event_type_pattern_changed;
    event_data.pattern_changed.plugin = m.proxy;
    event_data.pattern_changed.index = index;
    return true;
}

bool op_pattern_replace::operate(zzub::song& song) {
    return true;
}

void op_pattern_replace::finish(zzub::song& song, bool send_events) {
    if (send_events) song.plugin_invoke_event(0, event_data, true);
}

// ---------------------------------------------------------------------------
//
// op_pattern_insert_rows
//
// ---------------------------------------------------------------------------

op_pattern_insert_rows::op_pattern_insert_rows(int _id, int _index, int _row, std::vector<int> _columns, int _count) {
    id = _id;
    index = _index;
    columns = _columns;
    row = _row;
    count = _count;

    copy_flags.copy_graph = true;
    copy_flags.copy_plugins = true;

    operation_copy_plugin_flags pluginflags;
    pluginflags.plugin_id = id;
    pluginflags.copy_plugin = true;
    pluginflags.copy_patterns = true;
    copy_flags.plugin_flags.push_back(pluginflags);

}

bool op_pattern_insert_rows::prepare(zzub::song& song) {
    assert(id < song.plugins.size());
    assert(song.plugins[id] != 0);

    metaplugin& m = *song.plugins[id];

    zzub::pattern& p = *m.patterns[index];

    for (size_t i = 0; i < columns.size() / 3; i++) {
        int group = columns[i * 3 + 0];
        int track = columns[i * 3 + 1];
        int column = columns[i * 3 + 2];
        zzub::pattern::column& patterncolumn = p.groups[group][track][column];
        const zzub::parameter* param = song.plugin_get_parameter_info(id, group, track, column);
        patterncolumn.insert(patterncolumn.begin() + row, count, param->value_none);
        patterncolumn.erase(patterncolumn.begin() + p.rows, patterncolumn.end());
    }

    event_data.type = event_type_pattern_insert_rows;
    event_data.pattern_insert_rows.plugin = m.proxy;
    event_data.pattern_insert_rows.column_indices = &columns.front();
    event_data.pattern_insert_rows.indices = columns.size();
    event_data.pattern_insert_rows.index = index;
    event_data.pattern_insert_rows.row = row;
    event_data.pattern_insert_rows.rows = count;
    return true;
}

bool op_pattern_insert_rows::operate(zzub::song& song) {
    return true;
}

void op_pattern_insert_rows::finish(zzub::song& song, bool send_events) {

    if (send_events) song.plugin_invoke_event(0, event_data, true);
}


// ---------------------------------------------------------------------------
//
// op_pattern_remove_rows
//
// ---------------------------------------------------------------------------

op_pattern_remove_rows::op_pattern_remove_rows(int _id, int _index, int _row, std::vector<int> _columns, int _count) {
    id = _id;
    index = _index;
    columns = _columns;
    row = _row;
    count = _count;

    copy_flags.copy_graph = true;
    copy_flags.copy_plugins = true;

    operation_copy_plugin_flags pluginflags;
    pluginflags.plugin_id = id;
    pluginflags.copy_plugin = true;
    pluginflags.copy_patterns = true;
    copy_flags.plugin_flags.push_back(pluginflags);

    operation_copy_pattern_flags patternflags;
    patternflags.plugin_id = id;
    patternflags.index = index;
    copy_flags.pattern_flags.push_back(patternflags);

}

bool op_pattern_remove_rows::prepare(zzub::song& song) {
    assert(id < song.plugins.size());
    assert(song.plugins[id] != 0);

    metaplugin& m = *song.plugins[id];

    zzub::pattern& p = *m.patterns[index];

    for (size_t i = 0; i < columns.size() / 3; i++) {
        int group = columns[i * 3 + 0];
        int track = columns[i * 3 + 1];
        int column = columns[i * 3 + 2];
        zzub::pattern::column& patterncolumn = p.groups[group][track][column];
        const zzub::parameter* param = song.plugin_get_parameter_info(id, group, track, column);
        patterncolumn.erase(patterncolumn.begin() + row, patterncolumn.begin() + row + count);
        patterncolumn.insert(patterncolumn.end(), count, param->value_none);
    }

    event_data.type = event_type_pattern_remove_rows;
    event_data.pattern_remove_rows.plugin = m.proxy;
    event_data.pattern_remove_rows.column_indices = &columns.front();
    event_data.pattern_remove_rows.indices = columns.size();
    event_data.pattern_remove_rows.row = row;
    event_data.pattern_remove_rows.rows = count;
    event_data.pattern_remove_rows.index = index;

    return true;
}

bool op_pattern_remove_rows::operate(zzub::song& song) {
    return true;
}

void op_pattern_remove_rows::finish(zzub::song& song, bool send_events) {
    if (send_events) song.plugin_invoke_event(0, event_data, true);
}

// ---------------------------------------------------------------------------
//
// op_sequencer_set_event
//
// ---------------------------------------------------------------------------

op_sequencer_set_event::op_sequencer_set_event(int _timestamp, int _track, int _action) {
    timestamp = _timestamp;
    track = _track;
    action = _action;

    copy_flags.copy_sequencer_tracks = true;
}

bool op_sequencer_set_event::prepare(zzub::song& song) {

    zzub::sequencer_track& seqtrack = song.sequencer_tracks[track];

    std::vector<sequence_event>::iterator pos = seqtrack.events.begin();
    int index = 0;

    while (seqtrack.events.size() && (size_t)index < seqtrack.events.size() && pos->time < timestamp) {
        pos++;
        index++;
    }

    if (seqtrack.events.size() == 0 || pos == seqtrack.events.end()) {
        if (action != -1) {
            sequence_event ev;
            ev.time = timestamp;
            ev.pattern_event.value = action;
            ev.pattern_event.length = 0;
            seqtrack.events.push_back(ev);
        }
    } else
        if (pos->time == timestamp) {
            if (action == -1) {
                seqtrack.events.erase(pos);
            } else {
                pos->pattern_event.value = action;
            }
        } else {
            if (action != -1 && pos != seqtrack.events.end()) {
                sequence_event ev;
                ev.time = timestamp;
                ev.pattern_event.value = action;
                ev.pattern_event.length = 0;
                seqtrack.events.insert(pos, ev);
            }
        }

    return true;
}

bool op_sequencer_set_event::operate(zzub::song& song) {
    return true;
}

void op_sequencer_set_event::finish(zzub::song& song, bool send_events) {
    if (send_events) {
        event_data.type = event_type_set_sequence_event;
        event_data.set_sequence_event.plugin = 0;
        event_data.set_sequence_event.track = track;
        event_data.set_sequence_event.time = timestamp;
        song.plugin_invoke_event(0, event_data, true);
    }
}

// ---------------------------------------------------------------------------
//
// op_sequencer_create_track
//
// ---------------------------------------------------------------------------

op_sequencer_create_track::op_sequencer_create_track(zzub::player* _player, int _id, sequence_type _type) {
    player = _player;
    id = _id;
    type = _type;
    copy_flags.copy_sequencer_tracks = true;
}

bool op_sequencer_create_track::prepare(zzub::song& song) {
    assert(id < song.plugins.size());
    assert(song.plugins[id] != 0);

    zzub::sequencer_track t {};
    t.plugin_id = id;
    t.type = type;
    t.proxy = new sequence_proxy(player, (int)song.sequencer_tracks.size());
    song.sequencer_tracks.push_back(t);
    return true;
}

bool op_sequencer_create_track::operate(zzub::song& song) {
    return true;
}

void op_sequencer_create_track::finish(zzub::song& song, bool send_events) {
    if (send_events) {  
        metaplugin& m = *song.plugins[id];
        
        event_data.pattern_remove_rows.plugin = m.proxy;
        event_data.type = event_type_set_sequence_tracks;
        event_data.set_sequence_tracks.plugin = 0;

        song.plugin_invoke_event(0, event_data, true);
    }
}


// ---------------------------------------------------------------------------
//
// op_sequencer_remove_track
//
// ---------------------------------------------------------------------------

op_sequencer_remove_track::op_sequencer_remove_track(int _track) {
    track = _track;
    copy_flags.copy_sequencer_tracks = true;
}

bool op_sequencer_remove_track::prepare(zzub::song& song) {
    if (track == -1) track = song.sequencer_tracks.size() - 1;
    assert(track >= 0 && track < song.sequencer_tracks.size());

    // adjust proxy object indices
    for (size_t i = 0; i < song.sequencer_tracks.size(); i++) {
        sequencer_track& seqtrack = song.sequencer_tracks[i];
        if (seqtrack.proxy->track > track) seqtrack.proxy->track--;
    }

    // after removing the events, finally let go of the plugin data
    song.sequencer_tracks.erase(song.sequencer_tracks.begin() + track);

    return true;
}

bool op_sequencer_remove_track::operate(zzub::song& song) {

    return true;
}

void op_sequencer_remove_track::finish(zzub::song& song, bool send_events) {
    event_data.type = zzub_event_type_sequencer_remove_track;
    if (send_events) song.plugin_invoke_event(0, event_data, true);
}


// ---------------------------------------------------------------------------
//
// op_sequencer_move_track
//
// ---------------------------------------------------------------------------

op_sequencer_move_track::op_sequencer_move_track(int _track, int _newtrack) {
    track = _track;
    newtrack = _newtrack;
    copy_flags.copy_sequencer_tracks = true;
}

bool op_sequencer_move_track::prepare(zzub::song& song) {
    if (track == -1) track = song.sequencer_tracks.size() - 1;
    if (newtrack == -1) newtrack = song.sequencer_tracks.size() - 1;
    assert(track >= 0 && track < song.sequencer_tracks.size());
    assert(newtrack >= 0 && newtrack < song.sequencer_tracks.size());

    if (track == newtrack) return true;

    sequencer_track trackcopy = song.sequencer_tracks[track];
    song.sequencer_tracks.erase(song.sequencer_tracks.begin() + track);
    if (song.sequencer_tracks.empty())
        song.sequencer_tracks.push_back(trackcopy); else
        song.sequencer_tracks.insert(song.sequencer_tracks.begin() + newtrack, trackcopy);

    // adjust proxy object indices
    for (size_t i = 0; i < song.sequencer_tracks.size(); i++) {
        sequencer_track& seqtrack = song.sequencer_tracks[i];
        if (seqtrack.proxy->track == track) {
            seqtrack.proxy->track = newtrack;
        } else
            if (newtrack < track && seqtrack.proxy->track >= newtrack && seqtrack.proxy->track < track) {
                seqtrack.proxy->track++;
            } else
                if (newtrack > track && seqtrack.proxy->track > track && seqtrack.proxy->track <= newtrack) {
                    seqtrack.proxy->track--;
                }
    }

    return true;
}

bool op_sequencer_move_track::operate(zzub::song& song) {
    return true;
}

void op_sequencer_move_track::finish(zzub::song& song, bool send_events) {
    event_data.type = event_type_set_sequence_tracks;
    if (send_events) song.plugin_invoke_event(0, event_data, true);
}


// ---------------------------------------------------------------------------
//
// op_sequencer_replace
//
// ---------------------------------------------------------------------------

op_sequencer_replace::op_sequencer_replace(const std::vector<sequencer_track>& _events) {
    tracks = _events;
    copy_flags.copy_sequencer_tracks = true;
}

bool op_sequencer_replace::prepare(zzub::song& song) {
    song.sequencer_tracks = tracks;
    return true;
}

bool op_sequencer_replace::operate(zzub::song& song) {
    return true;
}

void op_sequencer_replace::finish(zzub::song& song, bool send_events) {
    event_data.type = event_type_sequencer_changed;
    if (send_events) song.plugin_invoke_event(0, event_data, true);
}


// ---------------------------------------------------------------------------
//
// op_midimapping_insert
//
// ---------------------------------------------------------------------------

op_midimapping_insert::op_midimapping_insert(const zzub::midimapping& _midi_mapping) {
    midi_mapping = _midi_mapping;
    copy_flags.copy_midi_mappings = true;
}

bool op_midimapping_insert::prepare(zzub::song& song) {
    song.midi_mappings.push_back(midi_mapping);
    return true;
}

bool op_midimapping_insert::operate(zzub::song& song) {
    return true;
}


// ---------------------------------------------------------------------------
//
// op_midimapping_remove
//
// ---------------------------------------------------------------------------

op_midimapping_remove::op_midimapping_remove(int _index) {
    index = _index;
    copy_flags.copy_midi_mappings = true;
}

bool op_midimapping_remove::prepare(zzub::song& song) {
    song.midi_mappings.erase(song.midi_mappings.begin() + index);
    return true;
}

bool op_midimapping_remove::operate(zzub::song& song) {
    return true;
}


// ---------------------------------------------------------------------------
//
// op_wavetable_allocate_wavelevel
//
// ---------------------------------------------------------------------------

op_wavetable_allocate_wavelevel::op_wavetable_allocate_wavelevel(int _wave, int _level, int _sample_count, int _channels, wave_buffer_type _format) {
    wave = _wave;
    level = _level;
    sample_count = _sample_count;
    channels = _channels;
    format = _format;

    copy_flags.copy_wavetable = true;
    operation_copy_wavelevel_flags wavelevel_flags;
    wavelevel_flags.wave = wave;
    wavelevel_flags.level = level;
    wavelevel_flags.copy_samples = true;
    copy_flags.wavelevel_flags.push_back(wavelevel_flags);
}

bool op_wavetable_allocate_wavelevel::prepare(zzub::song& song) {
    wave_info_ex& w = *song.wavetable.waves[wave];
    w.allocate_level(level, sample_count, (wave_buffer_type)format, channels == 2 ? true : false);
    return true;
}

bool op_wavetable_allocate_wavelevel::operate(zzub::song& song) {
    return true;
}

void op_wavetable_allocate_wavelevel::finish(zzub::song& song, bool send_events) {
}


// ---------------------------------------------------------------------------
//
// op_wavetable_add_wavelevel
//
// ---------------------------------------------------------------------------

op_wavetable_add_wavelevel::op_wavetable_add_wavelevel(zzub::player* _player, int _wave) {
    player = _player;
    wave = _wave;
    copy_flags.copy_wavetable = true;
    operation_copy_wave_flags wave_flags;
    wave_flags.wave = wave;
    wave_flags.copy_wave = true;
    copy_flags.wave_flags.push_back(wave_flags);
}

bool op_wavetable_add_wavelevel::prepare(zzub::song& song) {
    wave_info_ex& w = *song.wavetable.waves[wave];
    wave_level_ex wl{};
    wl.proxy = new wavelevel_proxy(player, wave, w.levels.size());
    w.levels.push_back(wl);
    return true;
}

bool op_wavetable_add_wavelevel::operate(zzub::song& song) {
    return true;
}

void op_wavetable_add_wavelevel::finish(zzub::song& song, bool send_events) {
}


// ---------------------------------------------------------------------------
//
// op_wavetable_remove_wavelevel
//
// ---------------------------------------------------------------------------

op_wavetable_remove_wavelevel::op_wavetable_remove_wavelevel(int _wave, int _level) {
    wave = _wave;
    level = _level;
    copy_flags.copy_wavetable = true;
    operation_copy_wave_flags wave_flags;
    wave_flags.wave = wave;
    wave_flags.copy_wave = true;
    copy_flags.wave_flags.push_back(wave_flags);
}

bool op_wavetable_remove_wavelevel::prepare(zzub::song& song) {
    wave_info_ex& w = *song.wavetable.waves[wave];

    if (level == -1) level = w.levels.size() - 1;


    w.levels.erase(w.levels.begin() + level);

    event_data.type = event_type_delete_wave;
    event_data.delete_wave.wave = song.wavetable.waves[wave]->proxy;

    return true;
}

bool op_wavetable_remove_wavelevel::operate(zzub::song& song) {
    return true;
}

void op_wavetable_remove_wavelevel::finish(zzub::song& song, bool send_events) {
    //event_data.delete_wave.level = level;
    if (send_events) song.plugin_invoke_event(0, event_data, true);
}


// ---------------------------------------------------------------------------
//
// op_wavetable_move_wavelevel
//
// ---------------------------------------------------------------------------

op_wavetable_move_wavelevel::op_wavetable_move_wavelevel(int _wave, int _level, int _newlevel) {
    wave = _wave;
    level = _level;
    newlevel = _newlevel;
    copy_flags.copy_wavetable = true;
    operation_copy_wave_flags wave_flags;
    wave_flags.wave = wave;
    wave_flags.copy_wave = true;
    copy_flags.wave_flags.push_back(wave_flags);
}

bool op_wavetable_move_wavelevel::prepare(zzub::song& song) {
    wave_info_ex& w = *song.wavetable.waves[wave];

    wave_level_ex copylevel = w.levels[level];

    w.levels.erase(w.levels.begin() + level);
    if (w.levels.empty())
        w.levels.push_back(copylevel); else
        w.levels.insert(w.levels.begin() + newlevel, copylevel);
    return true;
}

bool op_wavetable_move_wavelevel::operate(zzub::song& song) {
    return true;
}

void op_wavetable_move_wavelevel::finish(zzub::song& song, bool send_events) {
}

// ---------------------------------------------------------------------------
//
// op_wavetable_wave_replace
//
// ---------------------------------------------------------------------------

op_wavetable_wave_replace::op_wavetable_wave_replace(int _wave, const wave_info_ex& _data) {
    wave = _wave;
    data = _data;

    copy_flags.copy_wavetable = true;
    operation_copy_wave_flags wave_flags;
    wave_flags.wave = wave;
    wave_flags.copy_wave = true;
    copy_flags.wave_flags.push_back(wave_flags);
}

bool op_wavetable_wave_replace::prepare(zzub::song& song) {
    // if stereo flag changes, we must reallocate all wavelevels so they are in the same format
    // but; this is very special
    wave_info_ex& w = *song.wavetable.waves[wave];
    w.fileName = data.fileName;
    w.name = data.name;
    w.volume = data.volume;
    w.flags = data.flags;
    w.envelopes = data.envelopes;
    //w.levels = data.levels;	// ikke s� lurt tror jeg
    // flags er bidir, stereo, og evt annet sinnsykt

    event_data.type = event_type_wave_changed;
    event_data.change_wave.wave = song.wavetable.waves[wave]->proxy;

    return true;
}

bool op_wavetable_wave_replace::operate(zzub::song& song) {
    return true;
}

void op_wavetable_wave_replace::finish(zzub::song& song, bool send_events) {
    if (send_events) song.plugin_invoke_event(0, event_data, true);
}


// ---------------------------------------------------------------------------
//
// op_wavetable_wavelevel_replace
//
// ---------------------------------------------------------------------------

op_wavetable_wavelevel_replace::op_wavetable_wavelevel_replace(int _wave, int _level, const wave_level_ex& _data) {
    wave = _wave;
    level = _level;
    data = _data;
    copy_flags.copy_wavetable = true;
    operation_copy_wave_flags wave_flags;
    wave_flags.wave = wave;
    wave_flags.copy_wave = true;
    copy_flags.wave_flags.push_back(wave_flags);
}

bool op_wavetable_wavelevel_replace::prepare(zzub::song& song) {
    wave_level_ex& l = song.wavetable.waves[wave]->levels[level];
    l.loop_start = data.loop_start;
    l.loop_end = data.loop_end;
    l.root_note = data.root_note;
    l.samples_per_second = data.samples_per_second;

    // update legacy values
    wave_info_ex& w = *song.wavetable.waves[wave];
    bool is_extended = w.get_extended();
    if (is_extended) {
        l.legacy_loop_start = w.get_unextended_samples(level, data.loop_start);
        l.legacy_loop_end = w.get_unextended_samples(level, data.loop_end);
    } else {
        l.legacy_loop_start = data.loop_start;
        l.legacy_loop_end = data.loop_end;
    }

    event_data.type = event_type_wave_changed;
    event_data.change_wave.wave = song.wavetable.waves[wave]->proxy;

    return true;
}

bool op_wavetable_wavelevel_replace::operate(zzub::song& song) {
    return true;
}

void op_wavetable_wavelevel_replace::finish(zzub::song& song, bool send_events) {
    if (send_events) song.plugin_invoke_event(0, event_data, true);
}


// ---------------------------------------------------------------------------
//
// op_wavetable_insert_sampledata
//
// ---------------------------------------------------------------------------

op_wavetable_insert_sampledata::op_wavetable_insert_sampledata(int _wave, int _level, int _pos) {
    wave = _wave;
    level = _level;
    pos = _pos;
    samples = 0;
    samples_length = 0;
    samples_format = wave_buffer_type_si16;
    samples_scale = 0;
    samples_channels = 0;
    copy_flags.copy_wavetable = true;
    operation_copy_wavelevel_flags wavelevel_flags;
    wavelevel_flags.wave = wave;
    wavelevel_flags.level = level;
    wavelevel_flags.copy_samples = true;
    copy_flags.wavelevel_flags.push_back(wavelevel_flags);
}

op_wavetable_insert_sampledata::~op_wavetable_insert_sampledata() {
    // Don't know if this is right?
    delete[] (int *)samples;
}


bool op_wavetable_insert_sampledata::prepare(zzub::song& song) {
    // we shall reallocate the backbuffer wave and insert the sample datas given to us

    wave_info_ex& w = *song.wavetable.waves[wave];
    wave_level_ex& l = song.wavetable.waves[wave]->levels[level];

    int bytes_per_sample = l.get_bytes_per_sample();
    int newsamples = l.sample_count + samples_length;
    int numsamples = l.sample_count;
    int channels = (w.flags & wave_flag_stereo) ? 2 : 1;
    int format = l.format;

    void* copybuffer = new char[bytes_per_sample * channels * numsamples];
    memcpy(copybuffer, w.get_sample_ptr(level), bytes_per_sample * channels * numsamples);

    bool allocw = w.allocate_level(level, newsamples, (wave_buffer_type)format, channels == 2 ? true : false);
    assert(allocw);

    void* dst = w.get_sample_ptr(level);
    CopySamples(copybuffer, dst, pos, format, format, channels, channels, 0, 0);
    CopySamples(samples, dst, samples_length, samples_format, format, samples_channels, channels, 0, pos * channels);
    CopySamples(copybuffer, dst, numsamples - pos, format, format, channels, channels, pos * channels, (pos + samples_length) * channels);

    if (channels == 2) {
        CopySamples(copybuffer, dst, pos, format, format, channels, channels, 1, 1);

        if (samples_channels == 2) {
            // copy stereo sample to stereo sample
            CopySamples(samples, dst, samples_length, samples_format, format, samples_channels, channels, 1, pos * channels + 1);
        } else {
            // copy mono sample to stereo sample
            CopySamples(samples, dst, samples_length, samples_format, format, samples_channels, channels, 0, pos * channels + 1);
        }

        CopySamples(copybuffer, dst, numsamples - pos, format, format, channels, channels, pos * channels + 1, (pos + samples_length) * channels + 1);

    }

    // Don't know if this is right?
    switch (format) {
    case 0:
        delete[] (int16_t *)copybuffer;
        break;
    case 1:
        delete[] (float *)copybuffer;
        break;
    case 2:
        delete[] (int32_t *)copybuffer;
        break;
    case 3:
        delete[] (S24 *)copybuffer;
        break;
    }

    event_data.type = event_type_wave_allocated;
    event_data.allocate_wavelevel.wavelevel = song.wavetable.waves[wave]->levels[level].proxy;

    return true;
}

bool op_wavetable_insert_sampledata::operate(zzub::song& song) {
    return true;
}

void op_wavetable_insert_sampledata::finish(zzub::song& song, bool send_events) {
    //event_data.allocate_wavelevel.level = level;
    if (send_events) song.plugin_invoke_event(0, event_data, true);
}



// ---------------------------------------------------------------------------
//
// op_wavetable_remove_sampledata
//
// ---------------------------------------------------------------------------

op_wavetable_remove_sampledata::op_wavetable_remove_sampledata(int _wave, int _level, int _pos, int _samples) {
    wave = _wave;
    level = _level;
    pos = _pos;
    samples = _samples;

    copy_flags.copy_wavetable = true;
    operation_copy_wavelevel_flags wavelevel_flags;
    wavelevel_flags.wave = wave;
    wavelevel_flags.level = level;
    wavelevel_flags.copy_samples = true;
    copy_flags.wavelevel_flags.push_back(wavelevel_flags);
}

bool op_wavetable_remove_sampledata::prepare(zzub::song& song) {
    wave_info_ex& w = *song.wavetable.waves[wave];
    wave_level_ex& l = song.wavetable.waves[wave]->levels[level];

    int bytes_per_sample = l.get_bytes_per_sample();
    int newsamples = l.sample_count - samples;
    int numsamples = l.sample_count;
    int channels = (w.flags & wave_flag_stereo) ? 2 : 1;
    int format = l.format;

    void* copybuffer = new char[numsamples * bytes_per_sample * channels];
    memcpy(copybuffer, w.get_sample_ptr(level), numsamples * bytes_per_sample * channels);

    // NOTE: this will delete[] live sample data unless wavelevel_copy_flags.copy_samples is set to true
    w.reallocate_level(level, newsamples);

    // copy non-erased parts of copybuffer back to sampledata
    void* dst = w.get_sample_ptr(level);
    CopySamples(copybuffer, dst, pos, format, format, channels, channels, 0, 0);
    CopySamples(copybuffer, dst, numsamples - (pos + samples), format, format, channels, channels, (pos + samples) * channels, pos * channels);

    if (channels == 2) {
        CopySamples(copybuffer, dst, pos, format, format, channels, channels, 1, 1);
        CopySamples(copybuffer, dst, numsamples - (pos + samples), format, format, channels, channels, (pos + samples) * channels + 1, pos * channels + 1);
    }
    // Don't know if this is right?
    switch (format) {
    case 0:
        delete[] (int16_t *)copybuffer;
        break;
    case 1:
        delete[] (float *)copybuffer;
        break;
    case 2:
        delete[] (int32_t *)copybuffer;
        break;
    case 3:
        delete[] (S24 *)copybuffer;
        break;
    }

    event_data.type = event_type_wave_allocated;
    event_data.allocate_wavelevel.wavelevel = song.wavetable.waves[wave]->levels[level].proxy;

    return true;
}

bool op_wavetable_remove_sampledata::operate(zzub::song& song) {
    return true;
}

void op_wavetable_remove_sampledata::finish(zzub::song& song, bool send_events) {
    //event_data.allocate_wavelevel.level = level;
    if (send_events) song.plugin_invoke_event(0, event_data, true);
}

} // namespace zzub
