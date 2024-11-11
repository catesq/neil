#pragma once

#include "libzzub/ports/port_facade.h"


namespace zzub {

class port_facade_plugin : public virtual zzub::plugin {
    port_facade ports{};

public:
    virtual ~port_facade_plugin() {};
    void init_port_facade(host* plugin_host, std::vector<zzub::port*> cv_ports);

    virtual int get_port_count();
    virtual zzub::port* get_port(int index);
    virtual int get_port_count(zzub::port_type type, zzub::port_flow flow);
    virtual zzub::port* get_port(zzub::port_type type, zzub::port_flow flow, int index);
};


}