#pragma once

#include "libzzub/metaplugin.h"
#include "libzzub/cv/connector.h"
#include "libzzub/cv/data.h"
#include "libzzub/cv/node.h"
#include "libzzub/host.h"

namespace zzub {


struct cv_target {
    cv_data* data;
    const cv_node& node;
    const cv_connector_opts& opts;

    cv_target(
        cv_data* data, 
        const cv_node& node,
        const cv_connector_opts& opts
    ) : data(data), node(node), opts(opts) {}


    static cv_target* create(
        cv_data* data, 
        const cv_node& node,
        const cv_connector_opts& opts
    );


    virtual void initialize(
        zzub::metaplugin& meta
    ) = 0;


    virtual void send(
        int numsamples
    ) = 0;
};



struct cv_target_zzub_audio : public cv_target {
    using cv_target::cv_target;

    virtual void initialize(
        zzub::metaplugin& meta
    ) override;
   

    virtual void send(
        int numsamples
    ) override;
};



struct cv_target_zzub_param : public cv_target {
    using cv_target::cv_target;

    virtual void initialize(
        zzub::metaplugin& meta
    ) override;
   

    virtual void send(
        int numsamples
    ) override;

private:
    zzub::host* host;
    const zzub::parameter* param_info;
    zzub::metaplugin_proxy* plugin_proxy;

    uint16_t param_type;
    uint16_t track_index;
    uint16_t param_index;
};



struct cv_target_port_param : public cv_target {
    using cv_target::cv_target;

    virtual void initialize(
        zzub::metaplugin& meta
    ) override;
   

    virtual void send(
        int numsamples
    ) override;

private:
    zzub::port* target_port;
};



struct cv_target_port_stream : public cv_target {
    using cv_target::cv_target;

    virtual void initialize(
        zzub::metaplugin& meta
    ) override;
   

    virtual void send(
        int numsamples
    ) override;

private:
    zzub::port* target_port;
};


}