#pragma once

#include <unordered_map>

#include "lv2_defines.h"
#include "Ports.h"
#include "PluginWorld.h"
#include "lv2_utils.h"
#include "zzub/zzub.h"
#include "zzub/plugin.h"



struct PluginInfo : zzub::info {
    PluginInfo(PluginWorld *world, const LilvPlugin *lilvPlugin);

    const PluginWorld* world;

    const LilvPlugin*  lilvPlugin;

    std::string        zzubUri;

    std::string        lv2Uri;

    std::string        lv2ClassUri;

    std::string        libraryPath;

    std::string        bundlePath;

    std::vector<Port*> ports;

    virtual      zzub::plugin* create_plugin() const;

    virtual bool store_info(zzub::archive *) const { return false; }

    void         add_generator_params();

private:
    void         build_ports();
    Port*        build_port(uint32_t index);

    PortType     get_port_type(const LilvPort* lilvPort);
    PortFlow     get_port_flow(const LilvPort* lilvPort);
};

