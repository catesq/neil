#include "libzzub/ports/port_plugin.h"


namespace zzub {

void
port_facade_plugin::init_port_facade(host* plugin_host, std::initializer_list<zzub::port*> port_list){
    std::vector<zzub::port*> cv_ports = port_list;
    ports.prepare(plugin_host, &cv_ports);
}


void
port_facade_plugin::init_port_facade(host* plugin_host, std::vector<zzub::port*>* cv_ports){
    ports.prepare(plugin_host, cv_ports);
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