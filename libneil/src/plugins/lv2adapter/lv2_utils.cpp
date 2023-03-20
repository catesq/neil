#include "lv2_utils.h"

#include "gtk/gtk.h"
#include "lv2_ports.h"
#include "zzub/plugin.h"

using boost::algorithm::trim;

extern "C" {
#include "ext/symap.h"
}

#include <mutex>

#include "lv2_defines.h"

// -----------------------------------------------------------------------

// forward declaration for function declarations blah
struct SharedCache;
struct lv2_zzub_info;
struct LilvZzubParamPort;

// -----------------------------------------------------------------------

// char* owned_lilv_str  a char* pointer which is owned & released by the lilv library
//     copy string/release a pointer to string -
std::string
free_string(char* owned_lilv_str) {
    std::string dup_str(owned_lilv_str);
    lilv_free(owned_lilv_str);
    return dup_str;
}

std::string
as_string(LilvNode* node, bool freeNode) {
    std::string val("");

    if (node == nullptr) {
        return val;
    }

    if (lilv_node_is_string(node)) {
        val.append(lilv_node_as_string(node));
    } else if (lilv_node_is_uri(node)) {
        val.append(lilv_node_as_uri(node));
    }

    if (freeNode) {
        lilv_node_free((LilvNode*)node);
    }

    return val;
}

std::string
as_string(const LilvNode* node) {
    return as_string((LilvNode*)node, false);
}

float as_float(const LilvNode* node, bool canFreeNode) {
    return as_numeric<float>((LilvNode*)node, canFreeNode);
}

int as_int(const LilvNode* node, bool canFreeNode) {
    return as_numeric<int>((LilvNode*)node, canFreeNode);
}

unsigned
scale_size(const LilvScalePoints* scale_points) {
    return (scale_points != nullptr) ? lilv_scale_points_size(scale_points) : 0;
}

unsigned
nodes_size(const LilvNodes* nodes) {
    return (nodes != nullptr) ? lilv_nodes_size(nodes) : 0;
}

// uint64_t get_plugin_type(const PluginWorld *world, const LilvPlugin *lilvPlugin);

extern "C" {
LV2_URID
map_uri(LV2_URID_Map_Handle handle, const char* uri) {
    lv2_lilv_world* cache = (lv2_lilv_world*)handle;
    const LV2_URID id = symap_map(cache->symap, uri);
    //        printf("mapped %u from %s\n", id, uri);
    return id;
}

const char*
unmap_uri(LV2_URID_Unmap_Handle handle, LV2_URID urid) {
    lv2_lilv_world* cache = (lv2_lilv_world*)handle;
    const char* uri = symap_unmap(cache->symap, urid);
    //        printf("unmapped %u to %s\n", urid, uri);
    return uri;
}

char* lv2_make_path(LV2_State_Make_Path_Handle handle, const char* path) {
    lv2_lilv_world* cache = (lv2_lilv_world*)handle;
    std::string fname = cache->hostParams.tempDir + FILE_SEPARATOR + std::string(path);
    return strdup(fname.c_str());
}
}

float get_ui_scale_factor(zzub::host* host) {
    GtkWidget* ui_window = (GtkWidget*)host->get_host_info()->host_ptr;
    return gtk_widget_get_scale_factor(ui_window);
}

std::string
describe_port_type(PortType type) {
    switch (type) {
        case PortType::BadPort:
            return "badport";
        case PortType::Audio:
            return "audio";
        case PortType::CV:
            return "cv";
        case PortType::Control:
            return "control";
        case PortType::Event:
            return "event";
        case PortType::Midi:
            return "midi";
        case PortType::Param:
            return "param";
    }
}

std::string
as_hex(u_int8_t byte) {
    static char hexchars[16]{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    return std::string({hexchars[(byte >> 4) & 0xf], hexchars[byte & 0xf]});
}

char* note_param_to_str(u_int8_t note_value, char* text) {
    text[0] = 'A' + (note_value & 0xFF);
    text[1] = '-';
    text[2] = '0' + std::min((note_value >> 4) & 0xFF, 9);
    text[3] = '\0';
    return text;
}

std::string
describe_midi_sys(uint8_t* data) {
    switch (data[0]) {
        case 0xf0:
            return "sys exclusive begin";
        case 0xf1:
            return "qtr frame";
        case 0xf2:
            return "song pos";
        case 0xf3:
            return "song select";
        case 0xf6:
            return "tune request";
        case 0xf8:
            return "msg clock";
        case 0xfa:
            return "start";
        case 0xfb:
            return "continue";
        case 0xfc:
            return "stop";
        case 0xfe:
            return "sense";
        case 0xff:
            return "reset";

        default:
            return "not sys msg";
    }
}

std::string
describe_midi_voice(uint8_t* data, uint8_t size) {
    switch (data[0] & 0xf0) {
        case 0x80:
            return "note off(" + DESCRIBE_CHAN(data) + ") " + DESCRIBE_NOTE_AND_VOL(data, size);
        case 0x90:
            return "note on(" + DESCRIBE_CHAN(data) + ") " + DESCRIBE_NOTE_AND_VOL(data, size);
        case 0xa0:
            return "aftertouch(" + DESCRIBE_CHAN(data) + ") " + DESCRIBE_NOTE_AND_VOL(data, size);
        case 0xb0:
            return "controller";
        case 0xc0:
            return "program change";
        case 0xd0:
            return "channel pressure" + DESCRIBE_CHAN(data) + DESCRIBE_NOTE_AND_VOL(data, size);
        case 0xe0:
            return "pitch bend";
        default:
            return "not voice msg";
    }
}

std::string
describe_midi(uint8_t* data, uint8_t size) {
    if (lv2_midi_is_voice_message(data)) {
        return describe_midi_voice(data, size);
    } else {
        return describe_midi_sys(data);
    }
}

bool is_distrho_event_out_port(lv2_port* port) {
    return (port->type == PortType::Event && port->flow == PortFlow::Output && port->name == "Events Output" && port->symbol == "lv2_events_out");
}

uint8_t
midi_msg_len(uint8_t cmd) {
    // this method only works for valid starting bytes of a short midi message
    assert(cmd >= 0x80 && cmd != 0xf0 && cmd != 0xf7);

    static const char messageLengths[] =
        {
            3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
            3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
            1, 2, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

    return messageLengths[cmd & 0x7f];
}

std::ostream&
operator<<(std::ostream& stream, const midi_msg& msg) {
    char str[20];

    sprintf(str, "cmd: %x, data: %x%x", msg.bytes[0], msg.bytes[1], msg.bytes[2]);
    stream << std::string(str);
    return stream;
}
