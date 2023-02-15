#include <string>
#include <ostream>

#include "lv2_zzub_info.h"
#include "lv2_adapter.h"
#include "lv2_ports.h"

inline void printport(const char *prefix, const LilvPlugin* lilvPlugin, const LilvPort* lilvPort, PortFlow flow) {
    printf("%s: Port '%s'. Plugin '%s'\nClasses:",
           prefix,
           as_string(lilv_port_get_name(lilvPlugin, lilvPort), true).c_str(),
           as_string(lilv_plugin_get_name(lilvPlugin), true).c_str()
    );
}


zzub::plugin *lv2_zzub_info::create_plugin() const {
    return new lv2_adapter((lv2_zzub_info*) &(*this));
}


lv2_zzub_info::lv2_zzub_info(lv2_lilv_world* cache, const LilvPlugin *lilvPlugin)
    : zzub::info(),
      lilvWorld(cache->lilvWorld),
      lilvPlugin(lilvPlugin),
      cache(cache)
{

    // gui's are created in invoke() after a double click
    //it's possible that the custom gui is not supported on gtk3
    LilvUIs* uis = lilv_plugin_get_uis(lilvPlugin);

    if(uis) 
    {
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

    add_attribute().set_name("auto midi note off if note playing and note length not set for that note")
                   .set_value_min(0)
                   .set_value_max(1)
                   .set_value_default(1);

    printf("Registered plugin: name='%s', uri='%s', path='%s'\n", name.c_str(), uri.c_str(), bundlePath.c_str());

    PortCounter counter{};
    for(; counter.portIndex < lilv_plugin_get_num_ports(lilvPlugin); counter.portIndex++) {
        const LilvPort *lilvPort = lilv_plugin_get_port_by_index(lilvPlugin, counter.portIndex);

        PortFlow flow = get_port_flow(lilvPort);
        PortType type = get_port_type(lilvPort, flow);

        ports.push_back(build_port(lilvPort, flow, type, counter));

        flags |= ports.back()->get_zzub_flags();
    }

    zzubTotalDataSize = counter.dataOffset;

    if(lv2ClassUri == LV2_CORE__InstrumentPlugin) {
        zzub::midi_track_manager::add_midi_track_info(this);
        flags |= zzub::plugin_flag_is_instrument;
    } else if (flags & zzub::plugin_flag_has_audio_output) {
        flags |= zzub::plugin_flag_is_effect;
    } else {
        flags |= zzub::plugin_flag_control_plugin;
    }
}



PortFlow 
lv2_zzub_info::get_port_flow(const LilvPort* port) 
{
   if(lilv_port_is_a(lilvPlugin, port, cache->nodes.port_input)) {
        return PortFlow::Input;
   } else if(lilv_port_is_a(lilvPlugin, port, cache->nodes.port_output)) {
       return PortFlow::Output;
   } else {
       return PortFlow::Unknown;
   }
}



PortType 
lv2_zzub_info::get_port_type(const LilvPort* lilvPort, PortFlow flow) 
{
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



lv2_port* 
lv2_zzub_info::build_port(const LilvPort* lilvPort, PortFlow flow, PortType type, PortCounter& counter) 
{
    switch(type) {
        case PortType::Control: {
            auto port = new control_port(lilvPort, lilvPlugin, cache, type, flow, counter);

            counter.control++;

            return port;
        }

        case PortType::Param: {
            auto port = new param_port(lilvPort, lilvPlugin, cache, type, flow, counter);

            counter.dataOffset += port->zzubValSize;
            counter.param++;

            global_parameters.push_back(&port->zzubParam);

            return port;
        }

        case PortType::Audio:
            return new audio_buf_port(lilvPort, lilvPlugin, cache, type, flow, counter);

        case PortType::CV:
            return new audio_buf_port(lilvPort, lilvPlugin, cache, type, flow, counter);

        case PortType::Event:
            return new event_buf_port(lilvPort, lilvPlugin, cache, type, flow, counter);

        case PortType::Midi:
            return new event_buf_port(lilvPort, lilvPlugin, cache, type, flow, counter);

        case PortType::BadPort:
            return new lv2_port(lilvPort, lilvPlugin, cache, type, flow, counter);
    }
}



// void 
// lv2_zzub_info::add_generator_params()
// {
//     add_track_parameter().set_note();

//     add_track_parameter().set_byte()
//                          .set_name("Track volume")
//                          .set_description("Volume (00-7f)")
//                          .set_value_min(zzub_volume_value_min)
//                          .set_value_max(zzub_volume_value_max)
//                          .set_value_none(zzub_volume_value_none)
//                          .set_value_default(0x40);

//     add_track_parameter().set_byte()
//                          .set_name("Midi command 1")
//                          .set_description("cmd(0xf0) chan(0x0f)")
//                          .set_value_min(0x80)
//                          .set_value_max(0xfe)
//                          .set_value_none(TRACKVAL_NO_MIDI_CMD)
//                          .set_value_default(TRACKVAL_NO_MIDI_CMD);

//     add_track_parameter().set_word()
//                          .set_name("Midi data 1")
//                          .set_description("byte1(0x7f00) byte2(0x007f)")
//                          .set_value_min(0)
//                          .set_value_max(0xfffe)
//                          .set_value_none(0xffff)
//                          .set_value_default(0);

//     add_track_parameter().set_byte()
//                          .set_name("Midi command 2")
//                          .set_description("status(0xf0) chan(0x0f)")
//                          .set_value_min(0x80)
//                          .set_value_max(0xfe)
//                          .set_value_none(TRACKVAL_NO_MIDI_CMD)
//                          .set_value_default(TRACKVAL_NO_MIDI_CMD);

//     add_track_parameter().set_word()
//                          .set_name("Midi data 2")
//                          .set_description("byte1(0x7f00) byte2(0x007f)")
//                          .set_value_min(0)
//                          .set_value_max(0xfffe)
//                          .set_value_none(0xffff)
//                          .set_value_default(0);
// }
