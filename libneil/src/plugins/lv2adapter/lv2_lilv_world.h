#pragma once

/*
 * Carla LV2 utils
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include <lv2/state/state.h>

#include <mutex>

#include "lilv/lilv.h"
#include "lv2/atom/forge.h"
#include "lv2_defines.h"

extern "C" {
#include "ext/symap.h"
}

// -----------------------------------------------------------------------

// struct PlaybackPosition {
//     uint32_t sampleRate;

//    uint32_t workPos;

//    float bar;
//    float barBeat;
//    float ticksPerBeat;
//    float beatsPerBar;
//    float beatsPerMinute;
//    bool playing = true;

// public:
//     PlaybackPosition() {}

//    PlaybackPosition& update(zzub::master_info *masterInfo, zzub::mixer *mixer) {
//        if(workPos == mixer->work_position) {
//            return *this;
//        }

//        workPos = mixer->song_position;

//    //    if(info.beatsPerMinute != masterInfo->beats_per_minute || info.beatsPerBar != masterInfo->beats_per_bar ) {
//    //    }
//        beatsPerMinute = masterInfo->beats_per_minute;
//        beatsPerBar = 4;
//        ticksPerBeat = masterInfo->ticks_per_beat;

//        float song_ticks = mixer->song_position / (float) masterInfo->samples_per_tick;
//        float song_beats = song_ticks / masterInfo->ticks_per_beat;

//        barBeat = std::fmod(song_beats, beatsPerBar);
//        bar = song_beats - barBeat;

//        return *this;
//    }
//};

struct symap_urids {
    uint32_t atom_Float;
    uint32_t atom_Int;
    uint32_t atom_Double;
    uint32_t atom_Long;
    uint32_t atom_Bool;
    uint32_t atom_Chunk;
    uint32_t atom_Sequence;
    uint32_t atom_eventTransfer;
    uint32_t maxBlockLength;
    uint32_t minBlockLength;
    uint32_t bufSeqSize;
    uint32_t nominalBlockLength;
    uint32_t log_Trace;
    uint32_t midi_MidiEvent;
    uint32_t param_sampleRate;

    uint32_t time_Position;
    uint32_t time_bar;
    uint32_t time_barBeat;
    uint32_t time_beatUnit;
    uint32_t time_beatsPerBar;
    uint32_t time_beatsPerMinute;
    uint32_t time_frame;
    uint32_t time_speed;
    uint32_t ui_updateRate;
    uint32_t ui_scaleFactor;
    uint32_t ui_transientWindowId;
    // u_int32_t patch_Set;
    // u_int32_t patch_property;
    // u_int32_t patch_value;
};

/////////
///
// // Plugin Types
// LilvNode *class_allpass;
// LilvNode *class_amplifier;
// LilvNode *class_analyzer;
// LilvNode *class_bandpass;
// LilvNode *class_chorus;
// LilvNode *class_comb;
// LilvNode *class_compressor;
// LilvNode *class_constant;
// LilvNode *class_converter;
// LilvNode *class_delay;
// LilvNode *class_distortion;
// LilvNode *class_dynamics;
// LilvNode *class_eq;
// LilvNode *class_envelope;
// LilvNode *class_expander;
// LilvNode *class_filter;
// LilvNode *class_flanger;
// LilvNode *class_function;
// LilvNode *class_gate;
// LilvNode *class_generator;
// LilvNode *class_highpass;
// LilvNode *class_instrument;
// LilvNode *class_limiter;
// LilvNode *class_lowpass;
// LilvNode *class_mixer;
// LilvNode *class_modulator;
// LilvNode *class_multiEQ;
// LilvNode *class_oscillator;
// LilvNode *class_paraEQ;
// LilvNode *class_phaser;
// LilvNode *class_pitch;
// LilvNode *class_reverb;
// LilvNode *class_simulator;
// LilvNode *class_spatial;
// LilvNode *class_spectral;
// LilvNode *class_utility;
// LilvNode *class_waveshaper;

// Unit Hints
// LilvNode *unit_name;
// LilvNode *unit_render;
// LilvNode *unit_symbol;
// LilvNode *unit_unit;

struct lv2_lilv_nodes {
    lv2_lilv_nodes(LilvWorld *world);
    ~lv2_lilv_nodes();

    LilvNode *port;
    LilvNode *symbol;
    LilvNode *designation;
    LilvNode *freeWheeling;
    LilvNode *reportsLatency;
    LilvNode *port_input;  // Port Types
    LilvNode *port_output;
    LilvNode *port_control;
    LilvNode *port_audio;
    LilvNode *port_cv;
    LilvNode *port_atom;
    LilvNode *port_event;
    LilvNode *port_midi;
    LilvNode *pprop_optional;  // Port Properties
    LilvNode *pprop_enumeration;
    LilvNode *pprop_integer;
    LilvNode *pprop_sampleRate;
    LilvNode *pprop_toggled;
    LilvNode *pprop_artifacts;
    LilvNode *pprop_continuousCV;
    LilvNode *pprop_discreteCV;
    LilvNode *pprop_expensive;
    LilvNode *pprop_strictBounds;
    LilvNode *pprop_logarithmic;
    LilvNode *pprop_notAutomatic;
    LilvNode *pprop_notOnGUI;
    LilvNode *pprop_trigger;
    LilvNode *pprop_nonAutomable;
    LilvNode *oldPropArtifacts;  // Deprecated in favour of the pprop variants but useful
    LilvNode *oldPropContinuousCV;
    LilvNode *oldPropDiscreteCV;
    LilvNode *oldPropExpensive;
    LilvNode *oldPropStrictBounds;
    LilvNode *oldPropLogarithmic;
    LilvNode *oldPropNotAutomatic;
    LilvNode *oldPropNotOnGUI;
    LilvNode *oldPropTrigger;
    LilvNode *ui_gtk2;  // UI Types
    LilvNode *ui_gtk3;
    LilvNode *ui_qt4;
    LilvNode *ui_qt5;
    LilvNode *ui_cocoa;
    LilvNode *ui_windows;
    LilvNode *ui_x11;
    LilvNode *ui_external;
    LilvNode *atom_bufferType;  // Misc
    LilvNode *atom_chunk;
    LilvNode *atom_sequence;
    LilvNode *atom_supports;
    LilvNode *state_state;
    LilvNode *value_default;
    LilvNode *value_minimum;
    LilvNode *value_maximum;
    LilvNode *midi_event;
    LilvNode *time_position;
    LilvNode *mm_defaultControl;  // MIDI CC
    LilvNode *mm_controlType;
    LilvNode *mm_controlNumber;
    LilvNode *worker_iface;
    LilvNode *worker_schedule;
    LilvNode *extensionData;
    LilvNode *showInterface;
    LilvNode *transientWindow;
    LilvNode *rdf_type;
};

// -----------------------------------------------------------------------
// Our LV2 World class

struct lv2_lilv_world {
    LilvWorld *lilvWorld;

    Symap *symap;

    LV2_State_Make_Path make_path;

    LV2_Atom_Forge forge;

    LV2_URID_Map map;

    LV2_URID_Unmap unmap;

    //    LV2_URI_Map_Feature uri_map;

    symap_urids urids;

    lv2_lilv_nodes nodes;

    lv2_host_params hostParams;

    // Base Types

    //    PlaybackPosition playbackPosition{};

    // -------------------------------------------------------------------

    ~lv2_lilv_world();

    void init_suil();
    void init_x_threads();

    const LilvPlugins *get_all_plugins() {
        return lilv_world_get_all_plugins(lilvWorld);
    }

    static lv2_lilv_world *get_instance() {
        static lv2_lilv_world instance{};
        return &instance;
    }

   private:
    static std::mutex suil_mtx;
    static bool suil_is_init;
    static bool are_threads_init;

    lv2_lilv_world();
};
