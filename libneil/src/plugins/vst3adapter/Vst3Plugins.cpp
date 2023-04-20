#include "Vst3Plugins.h"
#include <cstdlib>
#include "Vst3PluginInfo.h"

#include "public.sdk/source/vst/hosting/module.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "public.sdk/source/vst/hosting/plugprovider.h"

using Vst3Iterator = PluginInfoIterator<Vst3PluginInfo>;

extern "C" {
    using ModuleEntryFunc = bool (PLUGIN_API*) (void*);
    using ModuleExitFunc = bool (PLUGIN_API*) ();
    using PluginFactoryFunc = Steinberg::IPluginFactory* (PLUGIN_API*) ();
    typedef Steinberg::IPluginFactory* (PLUGIN_API *GetFactoryProc) ();
}



struct vst3_library_loader {
    vst3_library_loader(boost::filesystem::path lib_path) : lib_path(lib_path.string()) {}

    bool load() {
        void* dl_handle = dlopen(lib_path.c_str(), RTLD_LAZY);
        lib_handle.reset(dl_handle, [](void*){});

        if(!lib_handle)
            return false;
        module_entry = reinterpret_cast<ModuleEntryFunc> (dlsym(lib_handle.get(), "ModuleEntry"));
        module_exit = reinterpret_cast<ModuleExitFunc> (dlsym(lib_handle.get(), "ModuleExit"));

        auto factory_func = reinterpret_cast<GetFactoryProc> (dlsym(lib_handle.get(), "GetPluginFactory"));
        if(!factory_func)
            return false;

        if(module_entry)
            module_entry(lib_handle.get());

        auto factory_ptr = factory_func();
        if(!factory_ptr)
            return false;

        auto f = Steinberg::FUnknownPtr<Steinberg::IPluginFactory> (owned (factory_ptr));

        factory = VST3::Hosting::PluginFactory(f);

        return true;
    }

    const VST3::Hosting::PluginFactory& get_factory() {
        return factory;
    }

    ~vst3_library_loader() {
        factory = VST3::Hosting::PluginFactory{nullptr};

        if(lib_handle.use_count() > 1)
            return;

        if(module_exit)
            module_exit();

        dlclose(lib_handle.get());
        lib_handle.reset();
    }

private:
    std::shared_ptr<void> lib_handle;
    std::string lib_path;
    ModuleEntryFunc module_entry {nullptr};
    ModuleExitFunc module_exit {nullptr};
    VST3::Hosting::PluginFactory factory {nullptr};
};



void add_vst3(Vst3Iterator& self, boost::filesystem::path path) {
    vst3_library_loader lib{path};

    if(!lib.load())
        return;

    auto factory = lib.get_factory();

    for (auto& class_info : factory.classInfos ()) {
        if (class_info.category () == kVstAudioEffectClass) {
            Steinberg::Vst::PlugProvider* provider = new Steinberg::Vst::PlugProvider (factory, class_info, true);

            if(!provider)
                continue;

            self.add(new Vst3PluginInfo(path.string(), &class_info, provider));
        }
    }
}



bool can_add(boost::filesystem::path path) {
    return path.extension().string() == SHARED_LIBRARY_EXT;
}


Vst3PluginCollection::Vst3PluginCollection(const char* vst_path) {
    this->vst_path = vst_path ? vst_path : "~/.vst3";
}

void Vst3PluginCollection::initialize(zzub::pluginfactory *factory) {
    for(auto plugin_info: Vst3Iterator(&can_add, add_vst3, vst_path)) {
//        factory->register_info(plugin_info);
        printf("Registered vst3 plugin: %s\n", plugin_info->name.c_str());
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


zzub::plugincollection *zzub_get_plugincollection() {
    return new Vst3PluginCollection(std::getenv("VST3_PATH"));
}

const char *zzub_get_signature() { return ZZUB_SIGNATURE; }
