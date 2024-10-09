#include "libzzub/ports.h"

namespace zzub {

//



param_port::param_port(
    zzub::host* host,
    const zzub::parameter* param,
    int param_index
) : host(host)
  , param(param)
  , param_index(param_index)
{
}


//
port_flow
param_port::get_flow()
{
    return zzub::port_flow::input;
}


//
const char* param_port::get_name()
{
    return param->name;
}


//
port_type
param_port::get_type()
{
    return zzub::port_type::parameter;
}


//
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

//
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


//
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