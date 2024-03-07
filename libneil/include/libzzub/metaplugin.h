#pragma once

#include "libzzub/pattern.h"
#include "zzub/plugin.h"
#include "libzzub/graph.h"

#include <vector>

namespace zzub {

struct metaplugin_proxy;


// sequencer action values
enum sequencer_event_type {
    sequencer_event_type_none = -1,
    sequencer_event_type_mute = 0,
    sequencer_event_type_break = 1,
    sequencer_event_type_thru = 2,
    sequencer_event_type_pattern = 0x10,
};

struct metaplugin {
    zzub::plugin* plugin;
    plugin_descriptor descriptor;
    const zzub::info* info;
    int flags;
    host* callbacks;
    bool initialized;
    std::vector<std::vector<float>> work_buffer;
    std::string name;
    int tracks;
    sequencer_event_type sequencer_state;
    float x, y;
    bool is_muted, is_bypassed;
    pattern state_write;
    pattern state_last;
    pattern state_automation;

    std::string stream_source;

    int last_work_buffersize, last_work_frame;
    float last_work_max_left, last_work_max_right;
    bool last_work_audio_result;
    bool last_work_midi_result;
    double last_work_time;
    double cpu_load_time;
    int cpu_load_buffersize;
    double cpu_load;
    int writemode_errors;

    int midi_input_channel;
    std::vector<midi_message> midi_messages;

    std::vector<event_handler*> event_handlers;
    std::vector<pattern*> patterns;

    int note_group, note_column;
    int velocity_column;
    int wave_column;

    metaplugin_proxy* proxy;
};

}