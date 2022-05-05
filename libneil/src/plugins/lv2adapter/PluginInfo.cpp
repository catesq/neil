#include "PluginInfo.h"
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
    const char *flowtext = describe_port_flow(flow);

    printf("\nPort '%s'. Type '%s'. Direction '%s'. Plugin '%s'", 
           lilv::as_string(lilv_port_get_name(lilvPlugin, lilvPort), true).c_str(), 
           typetext, 
           flowtext,
           lilv::as_string(lilv_plugin_get_name(lilvPlugin), true).c_str()
    );
}

PluginInfo::PluginInfo(PluginWorld *world, const LilvPlugin *lilvPlugin)
    : zzub::info(),
      world(world),
      lilvPlugin(lilvPlugin) {

    // gui's are created in invoke() after a double click
    //it's possible that the custom gui is not supported on gtk3
    uis = lilv_plugin_get_uis(lilvPlugin);
    if(uis)
        flags |= zzub_plugin_flag_has_custom_gui;
    
    name = lilv::as_string(lilv_plugin_get_name(lilvPlugin), true);
    author = lilv::as_string(lilv_plugin_get_author_name(lilvPlugin), true);
    lv2Class = lilv::as_string(lilv_plugin_class_get_label(lilv_plugin_get_class(lilvPlugin)));

    libraryPath = lilv_uri_to_path(lilv::as_string(lilv_plugin_get_library_uri(lilvPlugin)).c_str());
    bundlePath = lilv_uri_to_path(lilv::as_string(lilv_plugin_get_bundle_uri(lilvPlugin)).c_str());
    lv2Uri = lilv::as_string(lilv_plugin_get_uri(lilvPlugin));
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

    // add_generator_params();


    int num_ports = lilv_plugin_get_num_ports(lilvPlugin);
    // defaultValues = (float*) calloc(num_ports, sizeof(float));
    // lilv_plugin_get_port_ranges_float(lilvPlugin, NULL, NULL, defaultValues);

    for(uint i = 0; i < num_ports; i++) {
        auto port = build_port(i);
        ports.push_back(port);
        portSymbol[port->symbol.c_str()] = port;
    }

    if(lv2Class == "Instrument") {
        printf("add generator params for %s\n", name);
        add_generator_params();
    }

    LilvNodes *extDataNodes = lilv_plugin_get_extension_data(lilvPlugin);
    LILV_FOREACH(nodes, dataIter, extDataNodes) {
        const LilvNode *lilvNode = lilv_nodes_get(extDataNodes, dataIter);
        if(verbose) printf("\nplugin has extension data: %s", lilv::as_string(lilvNode).c_str());
    }
    lilv_nodes_free(extDataNodes);

    printf("Registered plugin: uri='%s', path='%s'\n", uri.c_str(), bundlePath.c_str());
}

PluginInfo::~PluginInfo() {
    // free(defaultValues);
    lilv_uis_free(uis);
}
// void PluginInfo::setMinBufSize(unsigned size) {
//     if(size > world->hostParams.evbufMinSize) {
//         world->hostParams.evbufMinSize = size;
//     }

// //    evbufMinSize = MAX(size, evbufMinSize);
// //    world->updateHostParams();
// }
Port* PluginInfo::build_port(uint32_t idx) {
    const LilvPort *lilvPort = lilv_plugin_get_port_by_index(lilvPlugin, idx);
    PortFlow flow = PortFlow::Unknown;
    bool atomLv2Api = false;

    if(lilv_port_is_a(lilvPlugin, lilvPort, world->port_input)) {
        flow = PortFlow::Input;
    } else if(lilv_port_is_a(lilvPlugin, lilvPort, world->port_output)) {
        flow = PortFlow::Output;
    }

    if(lilv_port_is_a(lilvPlugin, lilvPort, world->port_control)) {
        if(flow == PortFlow::Input) {
            ParamPort *param_port = new ParamPort(this, lilvPort, flow, idx, paramPorts.size()+ controlPorts.size(), zzubGlobalsLen);
            zzubGlobalsLen += param_port->byteSize;
            flags |= zzub_plugin_flag_has_event_input;

        //portIdx2ControlInIdx.push_back(paramPorts.size());
            global_parameters.push_back(param_port->zzubParam);
            paramNames.push_back(param_port->name);
            paramPorts.push_back(param_port);
            return param_port;
        } else {
            
            ControlPort *control_port = new ControlPort(this, lilvPort, flow, idx, paramPorts.size()+ controlPorts.size());
            controlPorts.push_back(control_port);
            flags |= zzub_plugin_flag_has_event_output;
            return control_port;
        }
        

        // if(flow == PortFlow::Input) {
        //     ParamPort *param_port = new ParamPort(this, lilvPort, flow, idx, paramPorts.size(), zzubGlobalsLen);

        //     zzubGlobalsLen += param_port->byteSize;
        //     flags |= zzub_plugin_flag_has_event_input;

        //     //portIdx2ControlInIdx.push_back(paramPorts.size());
        //     global_parameters.push_back(param_port->zzubParam);
        //     paramNames.push_back(param_port->name);
        //     paramPorts.push_back(param_port);

        //     if(verbose)
        //         printport("control param", lilvPlugin, lilvPort, flow);

        //     return param_port;
        // }
    } else if (lilv_port_is_a(lilvPlugin, lilvPort, world->port_audio)) {
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

    } else if(lilv_port_is_a(lilvPlugin, lilvPort, world->port_cv)) {

        auto cv_port = new CvBufPort(this, lilvPort, flow, idx, cvPorts.size());
        cvPorts.push_back(cv_port);
        // if(verbose)
            // printport("cv port!", lilvPlugin, lilvPort, flow);
        flags |= (flow == PortFlow::Input) ? zzub_plugin_flag_has_cv_input : zzub_plugin_flag_has_cv_output;
        // add_cv_port(cv_port->name.c_str()).set_is_input(flow == PortFlow::Input).set_index(idx);

        return cv_port;

    } else if((atomLv2Api = lilv_port_is_a(lilvPlugin, lilvPort, world->port_atom)) || lilv_port_is_a(lilvPlugin, lilvPort, world->port_event)) {
        if (lilv_port_supports_event(lilvPlugin, lilvPort, world->midi_event)) {
            auto midi_port = new MidiPort(this, lilvPort, flow, idx, midiPorts.size());
            midiPorts.push_back(midi_port);
            flags |= (flow == PortFlow::Input) ? zzub_plugin_flag_has_midi_input : zzub_plugin_flag_has_midi_output;

            if(verbose)
                printport("midi port", lilvPlugin, lilvPort, flow);

            return midi_port;

        } else {
            auto event_port = new EventPort(this, lilvPort, flow, PortType::Event, idx, eventPorts.size());
            eventPorts.push_back(event_port);
            flags |= (flow == PortFlow::Input) ? zzub_plugin_flag_has_event_input : zzub_plugin_flag_has_event_output;

            if(verbose)
                printport("event", lilvPlugin, lilvPort, flow);

            return event_port;

        } 
        // int minBufSize = lilv::as_int(lilv_port_get(lilvPlugin, lilvPort, world->rz_minSize), true);
        // setMinBufSize(minBufSize);
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
