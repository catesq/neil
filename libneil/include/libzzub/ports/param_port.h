#pragma once

#include "libzzub/host.h"
#include "zzub/plugin.h"

namespace zzub {


/**
 * this wraps a zzub::parameter so a plugin can use ports_facade to use the parameter as
 * a port, for the cv feature - or as a parameter as before
 * 
 * the benefit is extra ports - like cv ports - can be added to old
 * zzub machines without affecting the existing parameters
 */
class param_port : public zzub::port {
    const zzub::parameter* param;
    zzub::host* host;
    int param_index;


public:

    //
    param_port(
        zzub::host* host,
        const zzub::parameter* param,
        int param_index
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