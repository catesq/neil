
#pragma once

#include "pugixml.hpp"
#include "libzzub/archive.h"
#include "libzzub/ccm_helpers.h"

#include "zzub/parameter.h"

namespace zzub {
using namespace pugi;


class CcmWriter {
    ArchiveWriter arch;
    
    xml_node saveParameter(xml_node &parent, const zzub::parameter &p);
    xml_node saveClasses(xml_node &parent, zzub::song &player);
    xml_node saveClass(xml_node &parent, const zzub::info &pl);
    xml_node saveHead(xml_node &parent, zzub::song &player);
    xml_node addMeta(xml_node &parent, const std::string &propname);
    xml_node saveSequencer(xml_node &parent, const std::string &id, zzub::song &seq);
    xml_node saveEventBindings(xml_node &parent, std::vector<zzub::event_connection_binding> &bindings);
    xml_node saveEventBinding(xml_node &parent, zzub::event_connection_binding &binding);
    xml_node saveParameterValue(xml_node &parent, std::string &group, int track, int param, int value);
    xml_node saveArchive(xml_node &parent, const std::string&, zzub::mem_archive &arc);
    xml_node saveInit(xml_node &parent, zzub::song &player, int plugin_id);
    xml_node saveAttributes(xml_node &parent, zzub::song &player, int plugin_id);
    xml_node savePatterns(xml_node &parent, zzub::song &player, int plugin_id);
    xml_node savePatternTrack(xml_node &parent, const std::string &colname, double fac, zzub::song &player, int plugin_id, zzub::pattern &p, int group, int track);
    xml_node savePattern(xml_node &parent, zzub::song &player, int plugin_id, zzub::pattern &p);
    xml_node saveSequence(xml_node &parent, double fac, zzub::song &player, int track);
    xml_node saveSequences(xml_node &parent, zzub::song &player, int plugin_id);
    xml_node saveMidiMappings(xml_node &parent, zzub::song &player, int plugin_id);
    xml_node savePlugin(xml_node &parent, zzub::song &player, int plugin_id);
    xml_node savePlugins(xml_node &parent, zzub::song &player);
    xml_node saveWave(xml_node &parent, zzub::wave_info_ex &info);
    xml_node saveWaves(xml_node &parent, zzub::song &player);
    xml_node saveEnvelope(xml_node &parent, zzub::envelope_entry& env);
    xml_node saveEnvelopes(xml_node &parent, zzub::wave_info_ex &info);
public:
    bool save(std::string fileName, zzub::player* player);
    bool saveSelected(std::string filename, zzub::player* player, const int* plugins, unsigned int size);
};

}