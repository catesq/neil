#include "VstPlugins.h"
#include <cstdlib>

extern "C" {
    // only used by VstPlugins class when searching for plugins in the vst folders. plugins tend to ask what version of vst the host supports (opcode 1).
    // not replying makes the plugin
    VstIntPtr VSTCALLBACK dummyHostCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstInt32 value, void *ptr, float opt) {
        return 2400;
    }
}

// lib is a shread librayr opened by boost::dll::shared_library() - it is an argument as lib is closed and all function pointer are invalid when lib goes out of scope
// vst_path is the name of the vst dll/.so
//AEffect* loadPlugin(boost::dll::shared_library lib, boost::filesystem::path vst_path, HostCallbackFunc hostCallback = nullptr) {
//    auto entryPoint =
//}


VstPlugins::VstPlugins(const char* vst_path) {
    this->vst_path = vst_path;
}

void VstPlugins::initialize(zzub::pluginfactory *factory) {
    for(auto& plugin_description: VstDescriptions(vst_path)) {
        printf("here now\n");
        printf("vst plugin: %s %s\n", plugin_description.name.c_str(), plugin_description.filename.c_str());
    }
//    LILV_FOREACH(plugins, iter, collection) {
//        const LilvPlugin *plugin = lilv_plugins_get(collection, iter);
//        PluginInfo *info = new PluginInfo(world, plugin);
//        factory->register_info(info);
//    }
    exit(0);
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
