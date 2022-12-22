#include "PluginInfo.h"
#include "PluginAdapter.h"
#include "Ports.h"
#include <string>
#include <ostream>


inline void printport(const char *prefix, const LilvPlugin* lilvPlugin, const LilvPort* lilvPort, PortFlow flow) {
    printf("%s: Port '%s'. Plugin '%s'\nClasses:",
           prefix,
           as_string(lilv_port_get_name(lilvPlugin, lilvPort), true).c_str(),
           as_string(lilv_plugin_get_name(lilvPlugin), true).c_str()
    );
}


zzub::plugin *PluginInfo::create_plugin() const {
    return new PluginAdapter((PluginInfo*) &(*this));
}

PluginInfo::PluginInfo(SharedCache *cache, const LilvPlugin *lilvPlugin)
    : zzub::info(),
      lilvWorld(cache->lilvWorld),
      lilvPlugin(lilvPlugin),
      cache(cache)
{

    // gui's are created in invoke() after a double click
    //it's possible that the custom gui is not supported on gtk3
    LilvUIs* uis = lilv_plugin_get_uis(lilvPlugin);

    if(uis) {
        flags |= zzub_plugin_flag_has_custom_gui;
        lilv_uis_free(uis);
    }

    name       = as_string(lilv_plugin_get_name(lilvPlugin), true);
    author     = as_string(lilv_plugin_get_author_name(lilvPlugin), true);

    libraryPath = free_string(lilv_file_uri_parse(as_string(lilv_plugin_get_library_uri(lilvPlugin)).c_str(), NULL));
    bundlePath  = free_string(lilv_file_uri_parse(as_string(lilv_plugin_get_bundle_uri(lilvPlugin)).c_str(), NULL));
    lv2Uri      = as_string(lilv_plugin_get_uri(lilvPlugin));
    lv2ClassUri = as_string(lilv_plugin_class_get_uri(lilv_plugin_get_class(lilvPlugin)));
    zzubUri     = std::string("@zzub.org/lv2adapter/") + (strncmp(lv2Uri.c_str(), "http://", 6) == 0 ? std::string(lv2Uri.substr(7)) : lv2Uri);

    uri = zzubUri.c_str();
    short_name.append(name);

    min_tracks = 1;
    max_tracks = 16;

    add_attribute().set_name("MIDI Channel (0=off)")
                   .set_value_min(0)
                   .set_value_max(16)
                   .set_value_default(0);

    add_attribute().set_name("Ui")
                   .set_value_min(0)
                   .set_value_max(1)
                   .set_value_default(1);

    printf("Registered plugin: name='%s', uri='%s', path='%s'\n", name.c_str(), uri.c_str(), bundlePath.c_str());

    for(uint32_t portIndex = 0, paramPortIndex=0, controlPortIndex=0; portIndex < lilv_plugin_get_num_ports(lilvPlugin); portIndex++) {
        auto port = build_port(portIndex, paramPortIndex, controlPortIndex);
        ports.push_back(port);
        flags |= mixin_plugin_flag(port);
    }

    if(lv2ClassUri == LV2_CORE__InstrumentPlugin) {
        add_generator_params();
        flags |= zzub::plugin_flag_is_instrument;
    } else if (flags & zzub::plugin_flag_has_audio_output) {
        flags |= zzub::plugin_flag_is_effect;
    } else {
        flags |= zzub::plugin_flag_control_plugin;
    }
}

PortFlow PluginInfo::get_port_flow(const LilvPort* port) {
   if(lilv_port_is_a(lilvPlugin, port, cache->nodes.port_input)) {
        return PortFlow::Input;
    } else if(lilv_port_is_a(lilvPlugin, port, cache->nodes.port_output)) {
        return PortFlow::Output;
    } else {
       return PortFlow::Unknown;
   }
}



PortType PluginInfo::get_port_type(const LilvPort* lilvPort, PortFlow flow) {
    if(lilv_port_is_a(lilvPlugin, lilvPort, cache->nodes.port_control)) {
        return flow == PortFlow::Input ? PortType::Param : PortType::Control;
    } else if (lilv_port_is_a(lilvPlugin, lilvPort, cache->nodes.port_audio)) {
        return PortType::Audio;
    } else if(lilv_port_is_a(lilvPlugin, lilvPort, cache->nodes.port_cv)) {
        return PortType::CV;
    } else if(lilv_port_is_a(lilvPlugin, lilvPort, cache->nodes.port_atom) ||
              lilv_port_is_a(lilvPlugin, lilvPort, cache->nodes.port_event) ||
              lilv_port_is_a(lilvPlugin, lilvPort, cache->nodes.port_midi)) {
        if (lilv_port_supports_event(lilvPlugin, lilvPort, cache->nodes.midi_event)) {
            return PortType::Midi;
        } else {
            return PortType::Event;
        }
    } else {
        printport("Unrecognised port", lilvPlugin, lilvPort, get_port_flow(lilvPort));
        return PortType::BadPort;
    }
}

uint32_t PluginInfo::mixin_plugin_flag(Port* port) {
    switch(port->type) {
    case PortType::BadPort: return 0;
    case PortType::Audio:   return port->flow == PortFlow::Input ? zzub_plugin_flag_has_audio_input : zzub_plugin_flag_has_audio_output;
    case PortType::CV:      return port->flow == PortFlow::Input ? zzub_plugin_flag_has_cv_input : zzub_plugin_flag_has_cv_output;
    case PortType::Event:   return port->flow == PortFlow::Input ? zzub_plugin_flag_has_event_input : zzub_plugin_flag_has_event_output;
    case PortType::Midi:    return port->flow == PortFlow::Input ? zzub_plugin_flag_has_midi_input : zzub_plugin_flag_has_midi_output;
    case PortType::Control: return zzub_plugin_flag_has_event_output;
    case PortType::Param:   return zzub_plugin_flag_has_event_input;
    }
}


Port* PluginInfo::build_port(uint32_t index, uint32_t& paramPortIndex, uint32_t& controlPortIndexCount) {
    const LilvPort *lilvPort = lilv_plugin_get_port_by_index(lilvPlugin, index);

    PortFlow flow = get_port_flow(lilvPort);
    PortType type = get_port_type(lilvPort, flow);

    switch(type) {
    case PortType::Control:
        return setup_control_val_port(new ControlPort(flow, index, controlPortIndexCount++), lilvPort);

    case PortType::Param:
        return setup_param_port(new ParamPort(flow, index, paramPortIndex++), lilvPort);

    case PortType::Audio:
    case PortType::CV:
        return setup_base_port(new AudioBufPort(type, flow, index), lilvPort);

    case PortType::Event:
    case PortType::Midi:
        return setup_base_port(new EventBufPort(type, flow, index), lilvPort);

    // unknown port type, use a dumb placeholder
    case PortType::BadPort:
        return setup_base_port(new Port(type, flow, index), lilvPort);
    }
}

Port* PluginInfo::setup_base_port(Port* port, const LilvPort* lilvPort) {
    port->properties = get_port_properties(cache, lilvPlugin, lilvPort);
    port->designation = get_port_designation(cache, lilvPlugin, lilvPort);

    port->name = as_string(lilv_port_get_name(lilvPlugin, lilvPort), true);
    port->symbol = as_string(lilv_port_get_symbol(lilvPlugin, lilvPort));

    return port;
}

Port* PluginInfo::setup_control_val_port(ValuePort* port, const LilvPort* lilvPort) {
    setup_base_port(port, lilvPort);

    LilvNode *default_val_node, *min_val_node, *max_val_node;
    lilv_port_get_range(lilvPlugin, lilvPort, &default_val_node, &min_val_node, &max_val_node);

    port->defaultValue = default_val_node != NULL ? as_float(default_val_node) : 0.f;
    port->minimumValue = min_val_node != NULL     ? as_float(min_val_node) : 0.f;
    port->maximumValue = min_val_node != NULL     ? as_float(max_val_node) : 0.f;

    if(port->defaultValue < port->minimumValue || port->defaultValue > port->maximumValue) {
        port->defaultValue = (port->maximumValue - port->minimumValue) / 2.0f;
    }

    return port;
}

Port* PluginInfo::setup_param_port(ParamPort* port, const LilvPort* lilvPort) {
    setup_control_val_port(port, lilvPort);

    port->zzubParam.flags = zzub::parameter_flag_state;


    LilvScalePoints *lilv_scale_points = lilv_port_get_scale_points(lilvPlugin, lilvPort);
    unsigned scale_points_size = scale_size(lilv_scale_points);

    if (LV2_IS_PORT_TOGGLED(port->properties) || LV2_IS_PORT_TRIGGER(port->properties)) {
        port->zzubParam.set_switch();
        port->zzubParam.value_default = zzub::switch_value_off;
    } else if(LV2_IS_PORT_ENUMERATION(port->properties)) {
        (scale_points_size <= 128) ?  port->zzubParam.set_byte() : port->zzubParam.set_word();
        port->zzubParam.value_default = (int) port->defaultValue;
        port->zzubParam.value_max = scale_points_size;
    } else if (LV2_IS_PORT_INTEGER(port->properties)) {
        (port->maximumValue - port->minimumValue <= 128) ? port->zzubParam.set_byte() : port->zzubParam.set_word();
        port->zzubParam.value_default = (int) port->defaultValue;
        port->zzubParam.value_min = 0;
        port->zzubParam.value_max = std::min((int)(port->maximumValue - port->minimumValue), 32768);
    } else {
        port->zzubParam.set_word();
        port->zzubParam.value_min = 0;
        port->zzubParam.value_max = 32768;
        port->zzubParam.value_default = port->lilv_to_zzub_value(port->defaultValue);
    }

    port->zzubParam.name        = port->name.c_str();
    port->zzubParam.description = port->zzubParam.name;
    port->zzubValSize           = port->zzubParam.get_bytesize();
    port->zzubValOffset         = zzubTotalDataSize;
    zzubTotalDataSize           += port->zzubValSize;

    if(lilv_scale_points != NULL ) {
        LILV_FOREACH(scale_points, spIter,lilv_scale_points) {
            const LilvScalePoint *lilvScalePoint = lilv_scale_points_get(lilv_scale_points, spIter);
            port->scalePoints.push_back(ScalePoint(lilvScalePoint));
            if(verbose) {
                auto last_item = port->scalePoints.size()-1;
                printf("\nScale point %f, %s", port->scalePoints[last_item].value, port->scalePoints[last_item].label.c_str());
            }
        }

        lilv_scale_points_free(lilv_scale_points);
    }

    global_parameters.push_back(&port->zzubParam);

    return port;
}


void PluginInfo::add_generator_params() {

    add_track_parameter().set_note();

    add_track_parameter().set_byte()
                         .set_name("Track volume")
                         .set_description("Volume (00-7f)")
                         .set_value_min(0)
                         .set_value_max(0x007F)
                         .set_value_none(TRACKVAL_VOLUME_UNDEFINED)
                         .set_value_default(0x40);

    add_track_parameter().set_byte()
                         .set_name("Midi command 1")
                         .set_description("cmd(0xf0) chan(0x0f)")
                         .set_value_min(0x80)
                         .set_value_max(0xfe)
                         .set_value_none(TRACKVAL_NO_MIDI_CMD)
                         .set_value_default(TRACKVAL_NO_MIDI_CMD);

    add_track_parameter().set_word()
                         .set_name("Midi data 1")
                         .set_description("byte1(0x7f00) byte2(0x007f)")
                         .set_value_min(0)
                         .set_value_max(0xfffe)
                         .set_value_none(0xffff)
                         .set_value_default(0);

    add_track_parameter().set_byte()
                         .set_name("Midi command 2")
                         .set_description("status(0xf0) chan(0x0f)")
                         .set_value_min(0x80)
                         .set_value_max(0xfe)
                         .set_value_none(TRACKVAL_NO_MIDI_CMD)
                         .set_value_default(TRACKVAL_NO_MIDI_CMD);

    add_track_parameter().set_word()
                         .set_name("Midi data 2")
                         .set_description("byte1(0x7f00) byte2(0x007f)")
                         .set_value_min(0)
                         .set_value_max(0xfffe)
                         .set_value_none(0xffff)
                         .set_value_default(0);
}

//Port* PluginInfo::port_by_symbol(std::string symbol) {
//    auto iter = portSymbol.find(symbol);

//    if(iter != portSymbol.end()) {
//        return iter->second;
//    }

//    return nullptr;
//}
