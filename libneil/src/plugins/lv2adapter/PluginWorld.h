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

#pragma once


#ifdef _WIN32
#    include <io.h>  /* for _mktemp */
#define FILE_SEPARATOR std::string("\\")
#else
#    include <sys/stat.h>
#    include <sys/types.h>
#    include <unistd.h>
#define FILE_SEPARATOR std::string("/")
#endif

#include "boost/algorithm/string.hpp"

using boost::algorithm::trim;

extern "C" {
  #include "ext/symap.h"
}

#include <mutex>
#include "lv2_defines.h"

#include "suil/suil.h"


// -----------------------------------------------------------------------

//forward declaration for function declarations blah
struct SharedCache;
struct PluginInfo;
struct LilvZzubParamPort;

// -----------------------------------------------------------------------

namespace {
    const char empty_str[1] = "";
}

// char* owned_lilv_str  a char* pointer which is owned & released by the lilv library
//     copy string/release a pointer to string -
inline std::string free_string(char* owned_lilv_str) {
    std::string dup_str(owned_lilv_str);
    lilv_free(owned_lilv_str);
    return dup_str;
}

inline std::string as_string(LilvNode *node, bool freeNode = false) {
    std::string val("");

    if(node == nullptr) {
        return val;
    }

    if(lilv_node_is_string(node)) {
        val.append(lilv_node_as_string(node));
    } else if(lilv_node_is_uri(node)) {
        val.append(lilv_node_as_uri(node));
    }

    if(freeNode) {
        lilv_node_free(node);
    }

    return val;
}

inline std::string as_string(const LilvNode *node) {
    return as_string((LilvNode*) node, false);
}

template<typename T> T as_numeric(LilvNode *node, bool canFreeNode = false) {
    T val = (T) 0;

    if(node == nullptr) {
        return val;
    }

    if (lilv_node_is_float(node)) {
        val = (T) lilv_node_as_float(node);
    } else if(lilv_node_is_int(node)) {
        val = (T) lilv_node_as_int(node);
    } else if (lilv_node_is_bool(node)) {
        val = (T) lilv_node_as_bool(node) ? 1 : 0;
    }

    if(canFreeNode) {
        lilv_node_free(node);
    }

    return val;
}

inline float as_float(const LilvNode *node, bool canFreeNode = false) {
    return as_numeric<float>((LilvNode*)node, canFreeNode);
}

inline float as_int(const LilvNode *node, bool canFreeNode = false) {
    return as_numeric<int>((LilvNode*)node, canFreeNode);
}

inline unsigned scale_size(const LilvScalePoints *scale_points) {
    return (scale_points != nullptr) ? lilv_scale_points_size(scale_points) : 0;
}

inline unsigned nodes_size(const LilvNodes *nodes) {
    return (nodes != nullptr) ? lilv_nodes_size(nodes) : 0;
}

//uint64_t get_plugin_type(const PluginWorld *world, const LilvPlugin *lilvPlugin);

extern "C" {
    const char* unmap_uri(LV2_URID_Unmap_Handle handle, LV2_URID urid);
    LV2_URID map_uri(LV2_URID_Map_Handle handle, const char *uri);
    char* lv2_make_path(LV2_State_Make_Path_Handle handle, const char *path);
}

// -----------------------------------------------------------------------
// Custom Atom types




//struct PlaybackPosition {
//    uint32_t sampleRate;

//    uint32_t workPos;

//    float bar;
//    float barBeat;
//    float ticksPerBeat;
//    float beatsPerBar;
//    float beatsPerMinute;
//    bool playing = true;

//public:
//    PlaybackPosition() {}

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


struct SymapUrids {
    u_int32_t atom_Float;
    u_int32_t atom_Int;
    u_int32_t atom_Double;
    u_int32_t atom_Long;
    u_int32_t atom_Bool;
    u_int32_t atom_Chunk;
    u_int32_t atom_Sequence;
    u_int32_t atom_eventTransfer;
    u_int32_t maxBlockLength;
    u_int32_t minBlockLength;
    u_int32_t bufSeqSize;
    u_int32_t nominalBlockLength;
    // u_int32_t log_Trace;
    u_int32_t midi_MidiEvent;
    u_int32_t param_sampleRate;
    // u_int32_t patch_Set;
    // u_int32_t patch_property;
    // u_int32_t patch_value;
    u_int32_t time_Position;
    u_int32_t time_bar;
    u_int32_t time_barBeat;
    u_int32_t time_beatUnit;
    u_int32_t time_beatsPerBar;
    u_int32_t time_beatsPerMinute;
    u_int32_t time_frame;
    u_int32_t time_speed;
    u_int32_t ui_updateRate;
    u_int32_t ui_scaleFactor;
    u_int32_t ui_transientWindowId;
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


struct Nodes {
    Nodes(LilvWorld* world);
    ~Nodes();

    LilvNode *port;
    LilvNode *symbol;
    LilvNode *designation;
    LilvNode *freeWheeling;
    LilvNode *reportsLatency;
    LilvNode *port_input;       // Port Types
    LilvNode *port_output;
    LilvNode *port_control;
    LilvNode *port_audio;
    LilvNode *port_cv;
    LilvNode *port_atom;
    LilvNode *port_event;
    LilvNode *port_midi;
    LilvNode *pprop_optional;     // Port Properties
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
    LilvNode *oldPropArtifacts;    //Deprecated in favour of the pprop variants but useful
    LilvNode *oldPropContinuousCV;
    LilvNode *oldPropDiscreteCV;
    LilvNode *oldPropExpensive;
    LilvNode *oldPropStrictBounds;
    LilvNode *oldPropLogarithmic;
    LilvNode *oldPropNotAutomatic;
    LilvNode *oldPropNotOnGUI;
    LilvNode *oldPropTrigger;
    LilvNode *ui_gtk2;   // UI Types
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

struct SharedCache {

    LilvWorld *lilvWorld;

    Nodes nodes;

    Lv2HostParams hostParams;
    LV2_URID_Map map;
    LV2_URID_Unmap unmap;
    LV2_State_Make_Path make_path;
//    LV2_URI_Map_Feature uri_map;

    Symap* symap;
    SymapUrids urids;

    LV2_Atom_Forge forge;

    std::mutex suil_mtx;
    bool suil_is_init;

    // Base Types


//    PlaybackPosition playbackPosition{};


    // -------------------------------------------------------------------

    ~SharedCache();

    void init_suil();


    const LilvPlugins *get_all_plugins() {
        return lilv_world_get_all_plugins(lilvWorld);
    }

    static SharedCache* getInstance() {
        static SharedCache instance{};
        return &instance;
    }

    // bool featureIsSupported(const char* uri);

    // const LV2_Feature** getLv2Features();

private:
    SharedCache();
};

