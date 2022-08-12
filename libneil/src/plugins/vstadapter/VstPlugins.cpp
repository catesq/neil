#include "VstPlugins.h"
#include <cstdlib>



VstPlugins::VstPlugins(const char* vst_path) {
    this->vst_path = vst_path;
}

void VstPlugins::initialize(zzub::pluginfactory *factory) {
    for(auto plugin_info: VstDescriptions(vst_path)) {
        factory->register_info(plugin_info);
        printf("registered vst2 plugin: %s\n", plugin_info->name.c_str());
    }
}

const zzub::info *VstPlugins::get_info(const char *uri, zzub::archive *data) {
    return 0;
}

const char *VstPlugins::get_uri() {
    return 0;
}

void VstPlugins::configure(const char *key, const char *value) {}

void VstPlugins::destroy() {
    delete this;
}


zzub::plugincollection *zzub_get_plugincollection() {
    const char* envpath = std::getenv("VST_PATH");

    if(envpath == NULL)
        envpath = "vst:.vst";

    return new VstPlugins(envpath);
}

const char *zzub_get_signature() { return ZZUB_SIGNATURE; }
