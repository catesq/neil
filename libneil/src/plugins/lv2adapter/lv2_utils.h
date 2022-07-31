#pragma once

#include <string>
#include <initializer_list>

#include "lv2_defines.h"
#include "zzub/plugin.h"

#define DESCRIBE_CHAN(data) std::to_string(data[0] & 0x0f)
#define DESCRIBE_NOTE(byte) std::to_string(byte)
#define DESCRIBE_VOL(byte)  std::to_string(byte)
#define DESCRIBE_NOTE_AND_VOL(data, size) ((size == 3) ? DESCRIBE_NOTE(data[1]) + "(" + DESCRIBE_VOL(data[2]) + ")" : DESCRIBE_NOTE(data[1]))

struct Port;

std::string
describe_port_type(PortType type);


float
get_ui_scale_factor(zzub::host* host);


std::string
as_hex(u_int8_t byte);


char *
note_param_to_str(u_int8_t note_value, char *text);


std::string
describe_midi_sys(uint8_t* data);


std::string
describe_midi_voice(uint8_t* data, uint8_t size);


std::string
describe_midi(uint8_t* data, uint8_t size);


uint8_t
midi_msg_len(uint8_t cmd);


bool
is_distrho_event_out_port(Port* port);

//use to read midi messages from the midi track column of the tracker, used in PluginAdapter and PluginInfo
#pragma pack(1)


union midi_msg {
    uint8_t bytes[3]{};
    struct {
        uint8_t  cmd;
        uint16_t data;
    } midi;
};

std::ostream& operator<<(std::ostream& stream, const midi_msg& msg);

struct trackvals {
    uint8_t note   = zzub::note_value_none;
    uint8_t volume = 0x40;
    midi_msg msg_1;
    midi_msg msg_2;
};

struct attrvals {
    int channel;
    int ui;
};


#pragma pack()


#define MIDI_NOTE_ON(chan) (uint8_t)(LV2_MIDI_MSG_NOTE_ON | (chan & 0x0F))
#define MIDI_NOTE_OFF(chan) (uint8_t)(LV2_MIDI_MSG_NOTE_OFF | (chan & 0x0F))
#define MIDI_NOTE(note) (uint8_t)((note >> 4) * 12 + (note & 0xf) - 1)
#define MIDI_CHAN_PRESSURE(chan) (uint8_t)(LV2_MIDI_MSG_CHANNEL_PRESSURE | (chan & 0x0F))
#define MIDI_KEY_PRESSURE(chan) (uint8_t)(LV2_MIDI_MSG_NOTE_PRESSURE | (chan & 0x0F))
#define MIDI_DATA(cmd) (uint8_t)(cmd & 0x7f)

struct MidiEvent {
    // LV2_Atom_Event event;
    uint64_t time;
    uint8_t size;
    uint8_t data[3];

    MidiEvent(uint64_t offset, std::initializer_list<uint8_t> bytes) {
        time = offset;
        size = bytes.size();

        uint8_t* dst = data;
        for(auto byte: bytes)
            *dst++ = byte;
    }

    MidiEvent(uint64_t offset, midi_msg& msg) {
        time = offset;
        size = midi_msg_len(msg.midi.cmd);

        data[0] = msg.midi.cmd;
        data[1] = (msg.midi.data & 0xff00) >> 8;
        data[2] = (msg.midi.data & 0x00ff);
    }

    std::string str() {
        std::string msg = "0x";
        for(int i=0; i<size; i++)
            msg += as_hex(data[i]) + "";

        return std::to_string(size) + ": " + msg + " " + describe_midi(data, size);
    }

};

struct MidiEvents {
    std::vector<MidiEvent> data{};

    MidiEvents()  {}

    template<typename ... Args>
    void add(uint64_t time, Args&& ... cmd_args) {
        add(MidiEvent(time, {std::forward<Args>(cmd_args)...}));
    }

    void add(MidiEvent&& evt) {
        data.push_back(evt);
    }

    void add_message(midi_msg msg) {
        data.emplace_back(0, msg);
    }

    void noteOn(uint8_t chan, uint8_t note, uint8_t velocity) {
		add(0, MIDI_NOTE_ON(chan), MIDI_NOTE(note), MIDI_DATA(velocity));
    }

    void noteOff(uint8_t chan, uint8_t note, uint8_t velocity) {
        add(0, MIDI_NOTE_OFF(chan), MIDI_NOTE(note), MIDI_DATA(velocity));
    }

    void noteOff(uint8_t chan, uint8_t note) {
        add(0, MIDI_NOTE_OFF(chan), MIDI_NOTE(note), (uint8_t) 0);
    }

    void aftertouch(uint8_t chan, uint8_t note, uint8_t velocity) {
        add(0, MIDI_KEY_PRESSURE(chan), MIDI_NOTE(note), MIDI_DATA(velocity));
    }

    void reset() {
        data.clear();
    }

    size_t count() {
        return data.size();
    }
};



