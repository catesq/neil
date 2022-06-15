#pragma once

#include <unordered_map>

#include "lv2_defines.h"
#include "Ports.h"
#include "PluginWorld.h"
#include "lv2_utils.h"
#include "zzub/zzub.h"
#include "zzub/plugin.h"



struct PluginInfo : zzub::info {
    PluginInfo(SharedCache *cache, const LilvPlugin *lilvPlugin);

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
    uint32_t     mixin_plugin_flag(Port* port);
    Port*        build_port(uint32_t index, uint32_t* paramPortCount, uint32_t* controlPortCount);

    Port*        setup_base_port(Port* port, const LilvPort* lilvPort);
    Port*        setup_control_val_port(ValuePort* port,const LilvPort* lilvPort);
    Port*        setup_param_port(ParamPort* port,const LilvPort* lilvPort);

    PortType     get_port_type(const LilvPort* lilvPort, PortFlow flow);
    PortFlow     get_port_flow(const LilvPort* lilvPort);
};

