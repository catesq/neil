#include "PluginInfo.h"
#include "PluginAdapter.h"
#include "Ports.h"
#include <string>
#include <ostream>

#include "suil/suil.h"

inline const char* describe_port_flow(PortFlow flow) {
    if(flow == PortFlow::Input)
        return "Input";
    else if(flow == PortFlow::Output)
        return "Output";
    else 
        return "Unknown";
}

inline void printport(const char *typetext, const LilvPlugin* lilvPlugin, const LilvPort* lilvPort, PortFlow flow) {
    printf("Port '%s'. Type '%s'. Direction '%s'. Plugin '%s'\nClasses:",
           as_string(lilv_port_get_name(lilvPlugin, lilvPort), true).c_str(),
           typetext, 
           describe_port_flow(flow),
           as_string(lilv_plugin_get_name(lilvPlugin), true).c_str()
    );
    auto portnodes = lilv_port_get_classes(lilvPlugin, lilvPort);
    LILV_FOREACH(nodes, i, portnodes) {
        const LilvNode* node = lilv_nodes_get(portnodes, i);
        printf("\n\turi: '%s', label: '%s'", as_string(node).c_str(), as_string(node).c_str());

    }
}


zzub::plugin *PluginInfo::create_plugin() const {
    return new PluginAdapter((PluginInfo*) &(*this));
}

PluginInfo::PluginInfo(PluginWorld *world, const LilvPlugin *lilvPlugin)
    : zzub::info(),
      world(world),
      lilvPlugin(lilvPlugin) {

    // gui's are created in invoke() after a double click
    //it's possible that the custom gui is not supported on gtk3
    LilvUIs* uis = lilv_plugin_get_uis(lilvPlugin);

    if(uis) {
        flags |= zzub_plugin_flag_has_custom_gui;
        lilv_uis_free(uis);
    }

    name = as_string(lilv_plugin_get_name(lilvPlugin), true);
    author = as_string(lilv_plugin_get_author_name(lilvPlugin), true);

    libraryPath = free_string(lilv_file_uri_parse(as_string(lilv_plugin_get_library_uri(lilvPlugin)).c_str(), NULL));
    bundlePath = free_string(lilv_file_uri_parse(as_string(lilv_plugin_get_bundle_uri(lilvPlugin)).c_str(), NULL));
    lv2Uri = as_string(lilv_plugin_get_uri(lilvPlugin));
    lv2ClassUri =  as_string(lilv_plugin_class_get_uri(lilv_plugin_get_class(lilvPlugin)));
    zzubUri = std::string("@zzub.org/lv2adapter/") + (strncmp(lv2Uri.c_str(), "http://", 6) == 0 ? std::string(lv2Uri.substr(7)) : lv2Uri);

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

    build_ports();

    if(lv2ClassUri == LV2_CORE__InstrumentPlugin) {
        add_generator_params();
        flags |= zzub::plugin_flag_is_instrument;
    } else if (flags & zzub::plugin_flag_has_audio_output) {
        flags |= zzub::plugin_flag_is_effect;
    } else {
        flags |= zzub::plugin_flag_control_plugin;
    }

    LilvNodes *extDataNodes = lilv_plugin_get_extension_data(lilvPlugin);
    LILV_FOREACH(nodes, dataIter, extDataNodes) {
        const LilvNode *lilvNode = lilv_nodes_get(extDataNodes, dataIter);
    }
    lilv_nodes_free(extDataNodes);

}

PortFlow PluginInfo::get_port_flow(const LilvPort* port) {
   if(lilv_port_is_a(lilvPlugin, port, world->nodes.port_input)) {
        return PortFlow::Input;
    } else if(lilv_port_is_a(lilvPlugin, port, world->nodes.port_output)) {
        return PortFlow::Output;
    } else {
       return PortFlow::Unknown;
   }
}

PortType PluginInfo::get_port_type(const LilvPort* lilvPort) {
    if(lilv_port_is_a(lilvPlugin, lilvPort, world->nodes.port_control)) {
        return PortType::Control;
    } else if (lilv_port_is_a(lilvPlugin, lilvPort, world->nodes.port_audio)) {
        return PortType::Audio;
    } else if(lilv_port_is_a(lilvPlugin, lilvPort, world->nodes.port_cv)) {
        return PortType::CV;
    } else if(lilv_port_is_a(lilvPlugin, lilvPort, world->nodes.port_atom) ||
              lilv_port_is_a(lilvPlugin, lilvPort, world->nodes.port_event) ||
              lilv_port_is_a(lilvPlugin, lilvPort, world->nodes.port_midi)) {
        if (lilv_port_supports_event(lilvPlugin, lilvPort, world->nodes.midi_event)) {
            return PortType::Midi;
        } else {
            return PortType::Event;
        }
    } else {
        printport("unrecognised port", lilvPlugin, lilvPort, get_port_flow(lilvPort));
        return PortType::None;
    }
}

void PluginInfo::build_ports() {
    uint32_t zzubDataOffset = 0;

    int num_ports = lilv_plugin_get_num_ports(lilvPlugin);
    for(uint i = 0; i < num_ports; i++) {
        auto port = build_port(i);
        ports.push_back(port);
        portSymbol[port->symbol.c_str()] = port;
    }
}

Port* PluginInfo::build_port(uint32_t idx) {
    const LilvPort *lilvPort = lilv_plugin_get_port_by_index(lilvPlugin, idx);


    auto name = as_string(lilv_port_get_symbol(lilvPlugin, lilvPort));

//    printf("Port number: %d. Name: %s \n", idx, name.c_str());
    PortFlow flow = get_port_flow(lilvPort);
    PortType type = get_port_type(lilvPort);



    if(lilv_port_is_a(lilvPlugin, lilvPort, world->nodes.port_control)) {
        if(flow == PortFlow::Input) {
            ParamPort *param_port = new ParamPort(this, lilvPort, flow, idx, paramPorts.size(), zzubGlobalsLen);
            zzubGlobalsLen += param_port->byteSize;
            flags |= zzub_plugin_flag_has_event_input;

            global_parameters.push_back(param_port->zzubParam);
            paramNames.push_back(param_port->name);
            paramPorts.push_back(param_port);
            return param_port;
        } else {
            
            ControlPort *control_port = new ControlPort(this, lilvPort, flow, idx, controlPorts.size());
            controlPorts.push_back(control_port);
            flags |= zzub_plugin_flag_has_event_output;
            return control_port;
        }

    } else if (lilv_port_is_a(lilvPlugin, lilvPort, world->nodes.port_audio)) {
        auto audio_port = new AudioBufPort(this, lilvPort, flow, idx, audioPorts.size());

        audioPorts.push_back(audio_port);
        if(verbose)
            printport("audio port", lilvPlugin, lilvPort, flow);

        flags |= (flow == PortFlow::Input) ? zzub_plugin_flag_has_audio_input : zzub_plugin_flag_has_audio_output;

        if (flow == PortFlow::Input) {
            flags |= zzub_plugin_flag_has_audio_input;
            audio_in_count++;
        } else {
            flags |= zzub_plugin_flag_has_audio_output;
            audio_out_count++;
        }

        return audio_port;

    } else if(lilv_port_is_a(lilvPlugin, lilvPort, world->nodes.port_cv)) {

        auto cv_port = new CvBufPort(this, lilvPort, flow, idx, cvPorts.size());
        cvPorts.push_back(cv_port);
        // if(verbose)
            // printport("cv port!", lilvPlugin, lilvPort, flow);
        flags |= (flow == PortFlow::Input) ? zzub_plugin_flag_has_cv_input : zzub_plugin_flag_has_cv_output;
        // add_cv_port(cv_port->name.c_str()).set_is_input(flow == PortFlow::Input).set_index(idx);

        return cv_port;

    } else if(lilv_port_is_a(lilvPlugin, lilvPort, world->nodes.port_atom) ||
              lilv_port_is_a(lilvPlugin, lilvPort, world->nodes.port_event) ||
              lilv_port_is_a(lilvPlugin, lilvPort, world->nodes.port_midi)) {

        if (lilv_port_supports_event(lilvPlugin, lilvPort, world->nodes.midi_event)) {
            auto midi_port = new MidiPort(this, lilvPort, flow, idx, midiPorts.size());
            midiPorts.push_back(midi_port);
            flags |= (flow == PortFlow::Input) ? zzub_plugin_flag_has_midi_input : zzub_plugin_flag_has_midi_output;

//            if(verbose)
//                printport("midi port", lilvPlugin, lilvPort, flow);

            return midi_port;

        } else {
            auto event_port = new EventPort(this, lilvPort, flow, PortType::Event, idx, eventPorts.size());
            eventPorts.push_back(event_port);
            flags |= (flow == PortFlow::Input) ? zzub_plugin_flag_has_event_input : zzub_plugin_flag_has_event_output;

//            if(verbose)
//                printport("event", lilvPlugin, lilvPort, flow);

            return event_port;

        }

    } else {

        printport("unrecognised port", lilvPlugin, lilvPort, flow);

    }

    return new Port(this, lilvPort, flow, PortType::None, idx);
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

Port* PluginInfo::port_by_symbol(std::string symbol) {
    auto iter = portSymbol.find(symbol);

    if(iter != portSymbol.end()) {
        return iter->second;
    }

    return nullptr;
}
