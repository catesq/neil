#include <cstdlib>
#include "vst3_plugins.h"
#include "vst3_info.h"
#include "vst3_info_loader.h"


#include "public.sdk/source/vst/hosting/module.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "public.sdk/source/vst/hosting/plugprovider.h"


zzub::plugincollection *zzub_get_plugincollection() {
    return new Vst3PluginCollection(std::getenv("VST3_PATH"));
}


const char *zzub_get_signature() { return ZZUB_SIGNATURE; }


Vst3PluginCollection::Vst3PluginCollection(const char* vst_path) 
    : vst_path(vst_path ? vst_path : "vst3:.vst3"), 
      standardPluginContext(new Steinberg::Vst::HostApplication()) {
    Steinberg::Vst::PluginContextFactory::instance().setPluginContext(standardPluginContext);
}


void Vst3PluginCollection::initialize(zzub::pluginfactory *factory) {
    auto info_loader = Vst3InfoLoader();
    auto info_collector = PluginInfoIterator<struct Vst3Info>(info_loader, vst_path);

    for(auto plugin_info: info_collector.get_plugin_infos()) {
        printf("registered vst3 plugin: %s\n", plugin_info->name.c_str());
        factory->register_info(plugin_info);
    }
}


const zzub::info *Vst3PluginCollection::get_info(const char *uri, zzub::archive *data) {
    return 0;
}


const char *Vst3PluginCollection::get_uri() {
    return 0;
}


void Vst3PluginCollection::configure(const char *key, const char *value) {}


void Vst3PluginCollection::destroy() {
    delete this;
}
