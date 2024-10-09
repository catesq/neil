#pragma once

#include "libzzub/host.h"

#include "port_types.h"
#include "zzub/plugin.h"


namespace zzub {


/**
 * zzub plugins use this class to provide zzub::ports for the cv connection ui
 *
 */
class ports_facade {
    host* plugin_host;
    const info* plugin_info;

    std::vector<zzub::port*> audio_in_ports;
    std::vector<zzub::port*> audio_out_ports;
    std::vector<zzub::port*> param_ports;
    std::vector<zzub::port*> cv_in_ports;
    std::vector<zzub::port*> cv_out_ports;
    std::vector<zzub::port*> ports;

    std::vector<port*> build_param_ports();

    std::vector<port*> build_audio_ports(
        uint plugin_flag,
        zzub::port_flow port_flow,
        const std::string& prefix_name
    );


    std::vector<port*>& get_ports(
        zzub::port_type port_type, 
        zzub::port_flow port_flow
    );

public:
    ports_facade(
        host* plugin_host,
        std::vector<zzub::port*> cv_ports = {}
    );


    void
    process_events(const std::vector<port_event*>& events);


    //
    void
    init(zzub::archive* arc = nullptr)
    {
    }



    int
    get_port_count()
    {
        return ports.size();
    }


    int
    get_port_count(
        zzub::port_type port_type,
        zzub::port_flow port_flow
    )
    {
        return get_ports(port_type, port_flow).size();
    }


    //
    zzub::port*
    get_port(int index)
    {
        if (index < ports.size()) {
            return ports[index];
        } else {
            return nullptr;
        }
    }


    zzub::port*
    get_port(
        zzub::port_type port_type,
        zzub::port_flow port_flow,
        int index
    )
    {
        auto subports = get_ports(port_type, port_flow);

        if (index < subports.size()) {
            return subports[index];
        } else {
            return nullptr;
        }
    }
};


}