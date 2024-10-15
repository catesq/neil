#pragma once

#include <string>

#include "zzub/zzub.h"

namespace zzub {

inline const std::string ccm_version = "0.1";

enum {
    // Current version of the zzub interface. Pass this to the
    // version member of zzub::info.
    // It is not unlikely, that this enum argument is going
    // to be removed in future versions, since an automatic
    // signature system is now in place.
    version = zzub_version,

    // The maximum size of an audio buffer in samples. libzzub
    // will never pass a buffer that is larger than
    // zzub::buffer_size * sizeof(float) * 2 to any of the
    // plugins process methods.
    buffer_size = zzub_buffer_size
};

// Possible event types sent by the host. A plugin can register to
// receive these events in zzub::plugin::init() using the
// zzub::host::set_event_handler() method.
enum event_type {
    // Sent by the gui when the plugins visual representation
    // is being double clicked in the router. This is a good
    // moment to fire up an external gui.
    event_type_double_click = zzub_event_type_double_click,

    // most of the following events are used to organize library and gui
    // communication and should not be handled by a plugin.

    event_type_new_plugin = zzub_event_type_new_plugin,
    event_type_delete_plugin = zzub_event_type_delete_plugin,
    event_type_pre_delete_plugin = zzub_event_type_pre_delete_plugin,
    event_type_disconnect = zzub_event_type_disconnect,
    event_type_connect = zzub_event_type_connect,
    event_type_plugin_changed = zzub_event_type_plugin_changed,
    event_type_parameter_changed = zzub_event_type_parameter_changed,
    event_type_set_tracks = zzub_event_type_set_tracks,
    event_type_set_sequence_tracks = zzub_event_type_set_sequence_tracks,
    event_type_set_sequence_event = zzub_event_type_set_sequence_event,
    event_type_new_pattern = zzub_event_type_new_pattern,
    event_type_pre_delete_pattern = zzub_event_type_pre_delete_pattern,
    event_type_delete_pattern = zzub_event_type_delete_pattern,
    event_type_edit_pattern = zzub_event_type_edit_pattern,
    event_type_pattern_changed = zzub_event_type_pattern_changed,
    event_type_pre_disconnect = zzub_event_type_pre_disconnect,
    event_type_pre_connect = zzub_event_type_pre_connect,
    event_type_post_connect = zzub_event_type_post_connect,
    event_type_pre_set_tracks = zzub_event_type_pre_set_tracks,
    event_type_post_set_tracks = zzub_event_type_post_set_tracks,
    event_type_sequencer_add_track = zzub_event_type_sequencer_add_track,
    event_type_sequencer_remove_track = zzub_event_type_sequencer_remove_track,
    event_type_sequencer_changed = zzub_event_type_sequencer_changed,
    event_type_pattern_insert_rows = zzub_event_type_pattern_insert_rows,
    event_type_pattern_remove_rows = zzub_event_type_pattern_remove_rows,

    // global/master events
    event_type_load_progress = zzub_event_type_load_progress,
    event_type_midi_control = zzub_event_type_midi_control,
    event_type_wave_allocated = zzub_event_type_wave_allocated,

    event_type_player_state_changed = zzub_event_type_player_state_changed,
    event_type_osc_message = zzub_event_type_osc_message,

    event_type_envelope_changed = zzub_event_type_envelope_changed,
    event_type_slices_changed = zzub_event_type_slices_changed,
    event_type_wave_changed = zzub_event_type_wave_changed,
    event_type_delete_wave = zzub_event_type_delete_wave,

    // catch all event
    event_type_all = zzub_event_type_all
};

// Possible types for plugin parameters. These attributes can
// be passed to the type member of the zzub::parameter structure.
// They influence the size of data passed to the plugin,
// the visual appearance of parameter fields in the pattern
// editor and individual editing behaviour.
enum parameter_type {
    // Indicates that the parameter is a note, accepting
    // values from C-0 to B-9 and a special note off value. This
    // parameter has a size of 1 byte.
    parameter_type_note = zzub_parameter_type_note,

    // Indicates that the parameter is a switch, which
    // can either be on or off. This parameter has a size of 1
    // byte.
    parameter_type_switch = zzub_parameter_type_switch,

    // Indicates that the parameter is a byte, which can
    // take any random value between 0 and 255. This parameter
    // has a size of 1 byte.
    parameter_type_byte = zzub_parameter_type_byte,

    // Indicates that the parameter is a word, which can
    // take any random value between 0 and 65535. This parameter
    // has a size of 2 bytes.
    parameter_type_word = zzub_parameter_type_word
};

enum wave_buffer_type {
    wave_buffer_type_si16 = zzub_wave_buffer_type_si16,  // signed int 16bit
    wave_buffer_type_f32 = zzub_wave_buffer_type_f32,    // float 32bit
    wave_buffer_type_si32 = zzub_wave_buffer_type_si32,  // signed int 32bit
    wave_buffer_type_si24 = zzub_wave_buffer_type_si24   // signed int 24bit
};

enum note_value {
    // predefined values for notes
    note_value_none = zzub_note_value_none,
    note_value_off = zzub_note_value_off,
    note_value_min = zzub_note_value_min,
    note_value_max = zzub_note_value_max,
    note_value_c4 = zzub_note_value_c4
};

enum volume_value {
    volume_none = zzub_volume_value_none,
    volume_min = zzub_volume_value_min,
    volume_default = zzub_volume_value_default,
    volume_max = zzub_volume_value_max
};

enum switch_value {
    // predefined values for switches
    switch_value_none = zzub_switch_value_none,
    switch_value_off = zzub_switch_value_off,
    switch_value_on = zzub_switch_value_on
};

enum wavetable_index_value {
    // predefined values for wavetable indices
    wavetable_index_value_none = zzub_wavetable_index_value_none,
    wavetable_index_value_min = zzub_wavetable_index_value_min,
    wavetable_index_value_max = zzub_wavetable_index_value_max
};

enum parameter_flag {
    // parameter flags
    parameter_flag_wavetable_index = zzub_parameter_flag_wavetable_index,
    parameter_flag_state = zzub_parameter_flag_state,
    parameter_flag_event_on_edit = zzub_parameter_flag_event_on_edit
};

enum plugin_flag {
    // plugin flags
    plugin_flag_plays_waves = zzub_plugin_flag_plays_waves,
    plugin_flag_uses_lib_interface = zzub_plugin_flag_uses_lib_interface,
    plugin_flag_uses_instruments = zzub_plugin_flag_uses_instruments,
    plugin_flag_does_input_mixing = zzub_plugin_flag_does_input_mixing,
    plugin_flag_no_output = zzub_plugin_flag_no_output,
    plugin_flag_is_root = zzub_plugin_flag_is_root,
    plugin_flag_is_instrument = zzub_plugin_flag_is_instrument,
    plugin_flag_is_effect = zzub_plugin_flag_is_effect,
    plugin_flag_control_plugin = zzub_plugin_flag_control_plugin,
    plugin_flag_has_audio_input = zzub_plugin_flag_has_audio_input,
    plugin_flag_has_audio_output = zzub_plugin_flag_has_audio_output,
    plugin_flag_has_event_input = zzub_plugin_flag_has_event_input,
    plugin_flag_has_event_output = zzub_plugin_flag_has_event_output,
    plugin_flag_offline = zzub_plugin_flag_offline,
    plugin_flag_stream = zzub_plugin_flag_stream,
    plugin_flag_has_midi_input = zzub_plugin_flag_has_midi_input,
    plugin_flag_has_midi_output = zzub_plugin_flag_has_midi_output,
    plugin_flag_has_custom_gui = zzub_plugin_flag_has_custom_gui,
    plugin_flag_has_cv_input = zzub_plugin_flag_has_cv_input,
    plugin_flag_has_cv_output = zzub_plugin_flag_has_cv_output,
    plugin_flag_is_cv_generator = zzub_plugin_flag_is_cv_generator,
    plugin_flag_has_ports = zzub_plugin_flag_has_ports

};

enum state_flag {
    // player state flags
    state_flag_playing = zzub_state_flag_playing,
    state_flag_recording = zzub_state_flag_recording
};

enum wave_flag {
    wave_flag_loop = zzub_wave_flag_loop,
    wave_flag_extended = zzub_wave_flag_extended,
    wave_flag_stereo = zzub_wave_flag_stereo,
    wave_flag_pingpong = zzub_wave_flag_pingpong,
    wave_flag_envelope = zzub_wave_flag_envelope
};

enum envelope_flag {
    envelope_flag_sustain = zzub_envelope_flag_sustain,
    envelope_flag_loop = zzub_envelope_flag_loop
};

enum process_mode {
    // processing modes
    process_mode_no_io = zzub_process_mode_no_io,
    process_mode_read = zzub_process_mode_read,
    process_mode_write = zzub_process_mode_write,
    process_mode_read_write = zzub_process_mode_read_write
};

enum connection_type {
    connection_type_audio = zzub_connection_type_audio,
    connection_type_event = zzub_connection_type_event,
    connection_type_midi = zzub_connection_type_midi,
    connection_type_cv = zzub_connection_type_cv
};

enum sequence_type {
    sequence_type_pattern = zzub_sequence_type_pattern,
    sequence_type_wave = zzub_sequence_type_wave,
    sequence_type_automation = zzub_sequence_type_automation
};

}