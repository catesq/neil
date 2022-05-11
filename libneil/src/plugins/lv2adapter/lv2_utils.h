#pragma once

#include <string>
#include <initializer_list>

#include "lv2_defines.h"
#include "zzub/plugin.h"


inline std::string as_hex(u_int8_t byte) {
    static char hexchars[16] {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    return std::string({ hexchars[(byte >> 4) & 0xf], hexchars[byte & 0xf] });
}


inline char *note_param_to_str(u_int8_t note_value, char *text) {
    text[0] = ('A' + (note_value & 0xFF));
    text[1] = '-';
    text[2] = ('0' + std::min((note_value >> 4) & 0xFF, 9));
    text[3] = '\0';
    return text;
}

#define DESCRIBE_CHAN(data) std::to_string(data[0] & 0x0f)

#define DESCRIBE_NOTE(byte) std::to_string(byte)
#define DESCRIBE_VOL(byte) std::to_string(byte)
#define DESCRIBE_NOTE_AND_VOL(data, size) ((size == 3) ? DESCRIBE_NOTE(data[1]) + "(" + DESCRIBE_VOL(data[2]) + ")" : DESCRIBE_NOTE(data[1]))

inline std::string describe_midi_sys(uint8_t* data) {
    switch(data[0]) {
        case 0xf0: return "sys exclusive begin";
        case 0xf1: return "qtr frame";
        case 0xf2: return "song pos";
        case 0xf3: return "song select";
        case 0xf6: return "tune request";
        case 0xf8: return "msg clock";
        case 0xfa: return "start";
        case 0xfb: return "continue";
        case 0xfc: return "stop";
        case 0xfe: return "sense";
        case 0xff: return "reset";

        default: return "not sys msg";
    }
}

inline std::string describe_midi_voice(uint8_t* data, uint8_t size) {
    switch(data[0] & 0xf0) {
        case 0x80: return "note off(" + DESCRIBE_CHAN(data) + ") " +  DESCRIBE_NOTE_AND_VOL(data, size);
        case 0x90: return "note on(" + DESCRIBE_CHAN(data) + ") " +  DESCRIBE_NOTE_AND_VOL(data, size);
        case 0xa0: return "aftertouch(" + DESCRIBE_CHAN(data) + ") " +  DESCRIBE_NOTE_AND_VOL(data, size);
        case 0xb0: return "controller";
        case 0xc0: return "program change";
        case 0xd0: return "channel pressure" + DESCRIBE_CHAN(data) +  DESCRIBE_NOTE_AND_VOL(data, size);
        case 0xe0: return "pitch bend";
        default: return "not voice msg";
    }
}

inline std::string describe_midi(uint8_t* data, uint8_t size) {
    if(lv2_midi_is_voice_message(data)) {
        return describe_midi_voice(data, size);
    } else {
        return describe_midi_sys(data);
    }
}

//from JUCE MidiMessage::getMessageLengthFromFirstByte in midi_manager.h
inline uint8_t midi_msg_len(uint8_t cmd) {
    // this method only works for valid starting bytes of a short midi message
    assert (cmd >= 0x80 && cmd != 0xf0 && cmd != 0xf7);

    static const char messageLengths[] =
    {
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        1, 2, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    };

    return messageLengths[cmd & 0x7f];
}

//use to read midi messages from the midi track column of the tracker, used in PluginAdapter and PluginInfo
#pragma pack(1)

struct attrvals {
    int channel{};
    int ui{};
};

union midi_msg {
    uint8_t bytes[3]{};
    struct {
        uint8_t cmd{};
        uint16_t data{};
    } midi;
};

struct tvals {
    uint8_t note = zzub::note_value_none;
    uint8_t volume = 0x40;
    midi_msg msg_1;
    midi_msg msg_2;
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

    MidiEvent(uint64_t offset, std::initializer_list<uint8_t> _data) {
        time = offset;
        size = _data.size();
        uint8_t* dst = data;
        for(auto _byte: _data)
            *dst++ = _byte;
    }

    MidiEvent(uint64_t offset, midi_msg& msg) {
        time = offset;
        size = midi_msg_len(msg.midi.cmd);
        memcpy(data, msg.bytes, size);
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
