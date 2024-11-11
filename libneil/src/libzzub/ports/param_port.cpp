#include "libzzub/ports.h"
#include <thread>
#include <format>
namespace zzub {

//



param_port::param_port(
    zzub::host* host,
    const zzub::parameter* param,
    int param_index,
    zzub::port_flow flow
) : host(host)
  , param(param)
  , param_index(param_index),
    flow(flow)
{
}


/**
 * @return input or output
 */
port_flow
param_port::get_flow()
{
    return flow;
}


/**
 * @return name of the port
 */
const char* param_port::get_name()
{
    return param->name;
}


/**
 * @return zzub::port_type::param
 */
port_type
param_port::get_type()
{
    return zzub::port_type::param;
}



void 
param_port::set_value(
    float value
)
{
    host->set_parameter(
        host->_plugin,
        zzub_parameter_group_global,
        0,
        param_index,
        value
    );
}



void 
param_port::set_value(
    int value
)
{
    host->set_parameter(
        host->_plugin,
        zzub_parameter_group_global,
        0,
        param_index,
        value
    );
}


//
void
param_port::set_value(
    float* buf,
    uint count
)
{
    host->set_parameter(
        host->_plugin,
        zzub_parameter_group_global,
        0,
        param_index,
        buf[0]
    );
}




float
param_port::get_value()
{
    return host->get_parameter(
        host->_plugin,
        zzub_parameter_group_global,
        0,
        param_index
    );
}


}