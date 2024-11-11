#pragma once

#include <zzub/types.h>


namespace zzub
{




/*******************************************************************************************************
 *
 * cv_node_value
 *
 * node->value depends on node->port_type
 *
 * node type = audio:
 *     value is channel 0 or 1 -> left or right
 *
 * node_type = zzub_global_param_node:
 *     value is the index of the zzub_parameter in zzub_plugins globals
 *
 * node_type = zzub_track_param_node:
 *     upper 16 bits of value is track number
 *     lower 16 bits of param is the index of the parameter in that track
 *
 * node_type = ext_port_node
 *     value is index of the zzub_port (if that plugin supports zzub::port)
 *
 * node_type is midi_track_node
 *      value is ? - not supporting midi yet
 *
 *******************************************************************************************************/

union cv_node_value {
    uint32_t channels; // audio channels - bitmask up to 32 channels
    uint32_t index; // port or param
    struct {
        uint16_t track;
        uint16_t param;
    };
};


// represents a link to a plugin parameter, audio or cv port
struct cv_node {
    int32_t plugin_id = -1;

    // zzub::port_type - uses a uint32_t because of hassle with zzub.zidl
    uint32_t port_type;

    uint32_t value;

    // cv_node() : plugin_id(-1), type(audio_node), value({0}) {}
    // cv_node(int32_t plugin_id, uint32_t type, uint32_t value) : plugin_id(plugin_id), type(static_cast<cv_node_type>(type)), value({value}) {}

    bool operator==(const cv_node& other) const { return plugin_id == other.plugin_id && port_type == other.port_type && value == other.value; }
};






}
