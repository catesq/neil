#include "libzzub/ports.h"
#include "libzzub/ports/param_port.h"
#include "libzzub/ports/audio_port_facade.h"
#include <ranges>


namespace zzub {


void
ports_facade::init(
    host* plugin_host,
    std::vector<zzub::port*> cv_ports
)
{
    plugin_host = plugin_host;
    plugin_info = plugin_host->get_info(plugin_host->_plugin);
    param_ports = build_param_ports();
    
    audio_in_ports = build_audio_ports(
        zzub_plugin_flag_has_audio_input, 
        zzub::port_flow::input,
        "audio in {}"
    );

    audio_out_ports = build_audio_ports(
        zzub_plugin_flag_has_audio_output, 
        zzub::port_flow::output,
        "audio out {}"
    );

    std::ranges::copy_if(
        cv_ports,
        std::back_inserter(cv_in_ports),
        [](auto i) { return i->get_flow() == zzub::port_flow::input; }
    );

    std::ranges::copy_if(
        cv_ports,
        std::back_inserter(cv_out_ports),
        [](auto i) { return i->get_flow() == zzub::port_flow::output; }
    );

    for(std::vector<zzub::port*>* subports: {
        &audio_in_ports, &audio_out_ports, 
        &param_ports, &cv_in_ports, &cv_out_ports
    }) {
        ports.insert(
            ports.end(), 
            subports->begin(), 
            subports->end()
        );
    }
}


std::vector<zzub::port*>&
ports_facade::get_ports(
    zzub::port_type port_type, 
    zzub::port_flow port_flow
)
{
    switch(port_type)
    {
        case zzub::port_type::audio:
            if (port_flow == zzub::port_flow::input) {
                return audio_in_ports;
            } else {
                return audio_out_ports;
            }
            
        case zzub::port_type::param:
            return param_ports;

        case zzub::port_type::cv:
            if (port_flow == zzub::port_flow::input) {
                return cv_in_ports;
            } else {
                return cv_out_ports;
            }
    }

    throw std::runtime_error("unknown port type");
}



std::vector<zzub::port*>
ports_facade::build_param_ports()
{
    std::vector<zzub::port*> ports {};

    int index = 0;
    for (auto param : plugin_info->global_parameters) {
        ports.push_back(new param_port(plugin_host, param, index++));
    }

    return ports;
}




std::vector<zzub::port*>
ports_facade::build_audio_ports(
    uint test_plugin_flag, 
    zzub::port_flow port_flow, 
    const std::string& prefix_name
)
{
    std::vector<zzub::port*> ports {};

    if(plugin_info->flags & test_plugin_flag) {
        for(uint i = 0; i < 2; i++) {
            ports.push_back(new audio_port_facade(plugin_host, port_flow, "audio in {}", i));
        }
    }

    return ports;
}



void ports_facade::process_events(
    const std::vector<port_event*>& events
)
{
    for (auto event : events) {
        switch (event->event_type) {
        case port_event_type::port_event_type_value:
            param_ports[event->port_index]->set_value(event->value.f);
            break;
        }
    }
}


void
port_facade_plugin::init_port_facade(host* plugin_host, std::vector<zzub::port*> cv_ports){
    ports.init(plugin_host, cv_ports);
}



zzub::port* 
port_facade_plugin::get_port(int index)
{
    return ports.get_port(index);
}


int 
port_facade_plugin::get_port_count()
{
    return ports.get_port_count();
}


zzub::port* 
port_facade_plugin::get_port(
    zzub::port_type type, 
    zzub::port_flow flow, 
    int index
)
{
    return ports.get_port(type, flow, index);
}


int 
port_facade_plugin::get_port_count(
    zzub::port_type type, 
    zzub::port_flow flow
)
{
    return ports.get_port_count(type, flow);
}


}