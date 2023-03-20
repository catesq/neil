#pragma once

#include <boost/dll.hpp>
#include <string>

#include "aeffect.h"
#include "aeffectx.h"
#include "zzub/plugin.h"

#define MAX_TRACKS 16
#define MAX_EVENTS 256

#pragma pack(1)

struct attrvals {
    int channel;
    int keep_notes;
};

#pragma pack()

// from midi.lv2/midi.h
typedef enum {
    MIDI_MSG_INVALID = 0,             /**< Invalid Message */
    MIDI_MSG_NOTE_OFF = 0x80,         /**< Note Off */
    MIDI_MSG_NOTE_ON = 0x90,          /**< Note On */
    MIDI_MSG_NOTE_PRESSURE = 0xA0,    /**< Note Pressure */
    MIDI_MSG_CONTROLLER = 0xB0,       /**< Controller */
    MIDI_MSG_PGM_CHANGE = 0xC0,       /**< Program Change */
    MIDI_MSG_CHANNEL_PRESSURE = 0xD0, /**< Channel Pressure */
    MIDI_MSG_BENDER = 0xE0,           /**< Pitch Bender */
    MIDI_MSG_SYSTEM_EXCLUSIVE = 0xF0, /**< System Exclusive Begin */
    MIDI_MSG_MTC_QUARTER = 0xF1,      /**< MTC Quarter Frame */
    MIDI_MSG_SONG_POS = 0xF2,         /**< Song Position */
    MIDI_MSG_SONG_SELECT = 0xF3,      /**< Song Select */
    MIDI_MSG_TUNE_REQUEST = 0xF6,     /**< Tune Request */
    MIDI_MSG_CLOCK = 0xF8,            /**< Clock */
    MIDI_MSG_START = 0xFA,            /**< Start */
    MIDI_MSG_CONTINUE = 0xFB,         /**< Continue */
    MIDI_MSG_STOP = 0xFC,             /**< Stop */
    MIDI_MSG_ACTIVE_SENSE = 0xFE,     /**< Active Sensing */
    MIDI_MSG_RESET = 0xFF             /**< Reset */
} Midi_Message_Type;

// zzub to midi note
#define MIDI_NOTE(note) (uint8_t)((note >> 4) * 12 + (note & 0xf) - 1)

std::string get_plugin_string(AEffect* plugin, VstInt32 opcode, int index);

std::string get_param_name(AEffect* plugin, int index);

VstParameterProperties* get_param_props(AEffect* plugin, int index);

AEffect* load_vst(boost::dll::shared_library& lib, std::string vst_filename, AEffectDispatcherProc callback, void* user_p);

// typedef VstIntPtr (*DispatcherProc) (AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);

extern "C" {

// only used by VstPlugins class when searching for plugins in the vst folders. plugins tend to ask what version of vst the host supports (opcode 1).
VstIntPtr VSTCALLBACK dummyHostCallback(AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);
}

inline VstInt32 dispatch(AEffect* plugin, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt) {
    return ((AEffectDispatcherProc)plugin->dispatcher)(plugin, opcode, index, value, ptr, opt);
}

inline VstInt32 dispatch(AEffect* plugin, VstInt32 opcode) {
    return dispatch(plugin, opcode, 0, 0, 0, 0);
}

inline VstMidiEvent* vst_midi_event(std::array<uint8_t, 3> data) {
    auto event = new VstMidiEvent();
    memset(event, 0, sizeof(VstMidiEvent));

    event->type = kVstMidiType;
    event->byteSize = sizeof(VstMidiEvent);
    event->deltaFrames = 0;
    event->noteLength = 0;
    event->noteOffset = 0;
    memcpy(event->midiData, &data[0], 3);
    event->detune = 0;
    event->noteOffVelocity = 0;

    return event;
}

inline VstMidiEvent* midi_note_on(uint8_t note, uint8_t volume) {
    return vst_midi_event({MIDI_MSG_NOTE_ON, MIDI_NOTE(note), volume});
}

inline VstMidiEvent* midi_note_off(uint8_t note) {
    return vst_midi_event({MIDI_MSG_NOTE_OFF, MIDI_NOTE(note), 0});
}

inline VstMidiEvent* midi_note_aftertouch(uint8_t note, uint8_t volume) {
    return vst_midi_event({MIDI_MSG_NOTE_PRESSURE, MIDI_NOTE(note), volume});
}

inline VstMidiEvent* midi_message(uint8_t cmd, uint8_t data1, uint8_t data2) {
    return vst_midi_event({cmd, data1, data2});
}
