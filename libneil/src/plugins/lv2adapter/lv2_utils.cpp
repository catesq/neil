#include "lv2_utils.h"
#include "zzub/plugin.h"
#include "gtk/gtk.h"
#include "Ports.h"



float
get_ui_scale_factor(zzub::host* host) {
    GtkWidget* ui_window = (GtkWidget*) host->get_host_info()->host_ptr;
    return gtk_widget_get_scale_factor(ui_window);
}


std::string
describe_port_type(PortType type) {
    switch(type) {
    case PortType::BadPort: return "badport";
    case PortType::Audio:   return "audio";
    case PortType::CV:      return "cv";
    case PortType::Control: return "control";
    case PortType::Event:   return "event";
    case PortType::Midi:    return "midi";
    case PortType::Param:   return "param";
    }
}


std::string
as_hex(u_int8_t byte) {
    static char hexchars[16] {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    return std::string({ hexchars[(byte >> 4) & 0xf], hexchars[byte & 0xf] });
}


char *
note_param_to_str(u_int8_t note_value, char *text) {
    text[0] = 'A' + (note_value & 0xFF);
    text[1] = '-';
    text[2] = '0' + std::min((note_value >> 4) & 0xFF, 9);
    text[3] = '\0';
    return text;
}


std::string
describe_midi_sys(uint8_t* data) {
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


std::string
describe_midi_voice(uint8_t* data, uint8_t size) {
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


std::string
describe_midi(uint8_t* data, uint8_t size) {
    if(lv2_midi_is_voice_message(data)) {
        return describe_midi_voice(data, size);
    } else {
        return describe_midi_sys(data);
    }
}

bool
is_distrho_event_out_port(Port* port) {
    return (port->type == PortType::Event && port->flow == PortFlow::Output && port->name == "Events Output" && port->symbol == "lv2_events_out");
}

uint8_t
midi_msg_len(uint8_t cmd) {
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


std::ostream&
operator<<(std::ostream& stream, const midi_msg& msg) {
    char str[20];

    sprintf(str, "cmd: %x, data: %x%x", msg.bytes[0], msg.bytes[1], msg.bytes[2]);
    stream << std::string(str);
    return stream;
}


