#pragma once

#include <unordered_map>

#include "lv2_defines.h"
#include "Ports.h"
#include "PluginWorld.h"
#include "zzub/plugin.h"



struct PluginInfo : zzub::info {
    PluginInfo(SharedCache* cache, const LilvPlugin *lilvPlugin);

    const LilvWorld*    lilvWorld;

    const LilvPlugin*   lilvPlugin;

    SharedCache* cache;

    std::vector<Port*>  ports;

    std::string         zzubUri;

    std::string         lv2Uri;

    std::string         lv2ClassUri;

    std::string         libraryPath;

    std::string         bundlePath;

    unsigned            zzubTotalDataSize;

    virtual      zzub::plugin* create_plugin() const;
    virtual bool store_info(zzub::archive *) const { return false; }
    void         add_generator_params();

private:

    Port*        build_port(const LilvPort* lilvPort, PortFlow flow, PortType type, PortCounter& counter);

//    Port*        setup_base_port(Port* port, const LilvPort* lilvPort);
//    Port*        setup_control_val_port(ValuePort* port,const LilvPort* lilvPort);
//    Port*        setup_param_port(ParamPort* port,const LilvPort* lilvPort);

    PortType     get_port_type(const LilvPort* lilvPort, PortFlow flow);
    PortFlow     get_port_flow(const LilvPort* lilvPort);
};

