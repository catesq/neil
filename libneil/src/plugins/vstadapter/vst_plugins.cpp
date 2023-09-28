#include "vst_plugins.h"

#include <cstdlib>

#include "vst_adapter.h"



vst_plugins::vst_plugins(const char *vst_path) : 
    vst_path(vst_path ? vst_path : "vst:.vst") 
{
}


void vst_plugins::initialize(zzub::pluginfactory *factory) 
{
    for(auto plugin_info: PluginInfoIterator<vst_zzub_info, vst_info_loader>(vst_path).get_plugin_infos()) {
        factory->register_info(plugin_info);
    }
}


const zzub::info *vst_plugins::get_info(const char *uri, zzub::archive *data) {
    return 0;
}


const char *vst_plugins::get_uri() {
    return 0;
}


void vst_plugins::configure(const char *key, const char *value) {
}


void vst_plugins::destroy() {
    delete this;
}


zzub::plugincollection *
zzub_get_plugincollection() {
    return new vst_plugins(std::getenv("VST_PATH"));
}


const char *
zzub_get_signature() 
{
    return ZZUB_SIGNATURE;
}

