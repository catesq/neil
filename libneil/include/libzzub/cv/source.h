#pragma once

#include "libzzub/cv/node.h"

#include "libzzub/cv/data.h"
#include "libzzub/cv/connector.h"
#include "libzzub/metaplugin.h"
#include "libzzub/host.h"

namespace zzub
{


/**
 * cv_source objects are used in cv_transporter objects
 * 
 * each targeted port has one/several sources which all share one cv_data
 * each source updates the shared cv_data from the port/param it reads from in work() 
 */

struct cv_source {
    cv_data* data;
    cv_node node;
    cv_connector_opts opts;

    cv_source(
        cv_data* data,
        const cv_node& node,
        const cv_connector_opts& opts
    ) : data(data) , node(node) , opts(opts)
    {
    }


    virtual void initialize(
        zzub::metaplugin& from_plugin
    ) = 0;


    virtual void work(
        int numsamples
    ) = 0;


    static cv_source* create(
        cv_data* data, 
        const cv_node& node, 
        const cv_connector_opts& opts
    );
};




/*************************************************************************
 *
 * cv_source_mono_audio
 *
 ************************************************************************/


struct cv_source_mono_audio : public cv_source {
    using cv_source::cv_source;

    virtual void initialize(
        zzub::metaplugin& from_plugin
    );
    
    virtual void work(
        int numsamples
    );

private:
    float* src;
};




/*************************************************************************
 *
 * cv_source_zzub_param
 *
 ************************************************************************/

struct cv_source_zzub_param : public cv_source {
    using cv_source::cv_source;

    virtual void initialize(
        zzub::metaplugin& from_plugin
    );
    
    virtual void work(
        int numsamples
    );

private:
    zzub::host* host;
    const zzub::parameter* param_info;
    zzub::metaplugin_proxy* plugin_proxy;

    int raw_value;
    float norm_value;

    uint16_t param_type;
    uint16_t track_index;
    uint16_t param_index;
};



/*************************************************************************
 *
 * cv_source_port_param
 *
 ************************************************************************/

struct cv_source_port_param : public cv_source {
    using cv_source::cv_source;

    virtual void initialize(
        zzub::metaplugin& from_plugin
    );
    
    virtual void work(
        int numsamples
    );

protected:
    zzub::port* source_port = nullptr;
};



/*************************************************************************
 *
 * cv_source_port_stream
 *
 ************************************************************************/

struct cv_source_port_stream : public cv_source {
    using cv_source::cv_source;

    virtual void initialize(
        zzub::metaplugin& from_plugin
    );
    
    virtual void work(
        int numsamples
    );

protected:
    zzub::port* source_port = nullptr;
};


}