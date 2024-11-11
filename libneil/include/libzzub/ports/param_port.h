#pragma once

#include "libzzub/host.h"
#include "zzub/plugin.h"

namespace zzub {


/**
 * this wraps a zzub::parameter so the port facade can use zzub_paramaters 
 * each zzub paramater has two param_ports, one in - one out,
 * as merging the input and output ports conflicted with behaviour of lv2 ports  
 */
class param_port : public zzub::port {
public:
    zzub::host* host;
    const zzub::parameter* param;
    int param_index;
    zzub::port_flow flow;
    char* name;

public:

    //
    param_port(
        zzub::host* host,
        const zzub::parameter* param,
        int param_index,
        zzub::port_flow flow
    );


    //
    port_flow
    get_flow() override;


    //
    const char*
    get_name() override;


    //
    port_type
    get_type() override;


    //
    void
    set_value(
        float value
    ) override;


    //
    void
    set_value(
        int value
    ) override;


    //
    float
    get_value() override;


    //
    void
    set_value(
        float* buf,
        uint count
    ) override;
};


}