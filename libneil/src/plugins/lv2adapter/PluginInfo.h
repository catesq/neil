#pragma once

#include <unordered_map>

#include "lv2_defines.h"
#include "Ports.h"
#include "PluginWorld.h"
#include "lv2_utils.h"
#include "zzub/zzub.h"
#include "zzub/plugin.h"



struct PluginInfo : zzub::info {
	PluginWorld *world;
    const LilvPlugin *lilvPlugin;


    // float *defaultValues;

    LilvUIs *uis;

    // uint64_t pluginType = 0;
    int audio_in_count = 0;
    int audio_out_count = 0;

    unsigned zzubGlobalsLen = 0; // byte offset into the globals data populated by zzub
                                  // the globals data is a mix of 8 & 16 bit ints
    std::string zzubUri;
	std::string lv2Uri;
    std::string lv2ClassUri;
    std::string libraryPath;
    std::string bundlePath;

    std::vector<Port*> ports;
    std::vector<std::string> paramNames{};
    std::unordered_map<std::string, Port*> portSymbol{};

    std::vector<ControlPort *> controlPorts;
    std::vector<ParamPort *> paramPorts;

    std::vector<EventPort*> midiPorts{};

    std::vector<EventPort*> eventPorts{};

    std::vector<AudioBufPort*> audioPorts{};

    std::vector<CvBufPort*> cvPorts{};


    virtual zzub::plugin* create_plugin() const;
    virtual bool store_info(zzub::archive *) const { return false; }
    Port* port_by_symbol(std::string symbol);
    // void setMinBufSize(unsigned);

    PluginInfo(PluginWorld *world, const LilvPlugin *lilvPlugin);
    virtual ~PluginInfo();
    void add_generator_params();
//    uint32_t inline num_controls() { return controlPorts.size() + paramPorts.size(); }
private:
    Port *build_port(uint32_t index);
};

