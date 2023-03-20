#pragma once

#include <unordered_map>

#include "lv2_defines.h"
#include "lv2_lilv_world.h"
#include "lv2_ports.h"
#include "zzub/plugin.h"

struct lv2_zzub_info : zzub::info {
    lv2_zzub_info(lv2_lilv_world* cache, const LilvPlugin* lilvPlugin);

    const LilvWorld* lilvWorld;

    const LilvPlugin* lilvPlugin;

    lv2_lilv_world* cache;

    std::vector<lv2_port*> ports;

    std::string zzubUri;

    std::string lv2Uri;

    std::string lv2ClassUri;

    std::string libraryPath;

    std::string bundlePath;

    unsigned zzubTotalDataSize;

    virtual zzub::plugin* create_plugin() const;
    virtual bool store_info(zzub::archive*) const { return false; }
    void add_generator_params();

   private:
    lv2_port* build_port(const LilvPort* lilvPort, PortFlow flow, PortType type, PortCounter& counter);

    //    Port*        setup_base_port(Port* port, const LilvPort* lilvPort);
    //    Port*        setup_control_val_port(ValuePort* port,const LilvPort* lilvPort);
    //    Port*        setup_param_port(ParamPort* port,const LilvPort* lilvPort);

    PortType get_port_type(const LilvPort* lilvPort, PortFlow flow);
    PortFlow get_port_flow(const LilvPort* lilvPort);
};
