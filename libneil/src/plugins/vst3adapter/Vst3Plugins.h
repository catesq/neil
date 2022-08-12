#pragma once


#include "Vst3PluginInfo.h"

#include "zzub/plugin.h"
#include "zzub/signature.h"
#include "plugin_info_iterator.h"


void add_vst3(PluginInfoIterator<Vst3PluginInfo>& self, boost::filesystem::path path);


struct Vst3PluginCollection : zzub::plugincollection {
    Vst3PluginCollection(const char* vst_path);
    virtual void initialize(zzub::pluginfactory *factory);
    virtual const zzub::info *get_info(const char *uri, zzub::archive *data); // { return 0; }
    virtual const char *get_uri(); // { return 0; }
    virtual void configure(const char *key, const char *value); // {}
    virtual void destroy();
private:
    const char* vst_path;
};





