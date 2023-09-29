#pragma once

#include <boost/filesystem.hpp>

#include "aeffect.h"
#include "string.h"
#include "vst_defines.h"
#include "vst_plugin_info.h"
#include "zzub/plugin.h"
#include "zzub/signature.h"

#include "libzzub/plugin_info_iterator.h"

#if defined(WINOS)
#define strtok_r strtok_s
#endif

#if defined(POSIX)
#define VST_EXT ".so"
#elif defined(MACOS)
#define VST_EXT ".vst"
#elif defined(WINOS)
#define VST_EXT ".dll"
#else
#define VST_EXT ".???"  // ide does not pick up on the platform constants defined in scons, this is a kludge to silence some warnings about unknown constant
#endif

struct vst_info_loader: public zzub::plugin_info_loader<struct vst_zzub_info> {

    virtual bool is_plugin(boost::filesystem::path path) override {
        return boost::filesystem::is_regular_file(path) && path.extension().string() == VST_EXT;
    }

    virtual std::vector<struct vst_zzub_info*> get_plugin_infos(boost::filesystem::path path) override {
        auto infos = std::vector<struct vst_zzub_info*>{};

        boost::dll::shared_library lib{};
        AEffect* plugin = load_vst(lib, path.string(), dummyHostCallback, nullptr);

        if (plugin == nullptr)
            return infos;

        VstPlugCategory category = (VstPlugCategory)dispatch(plugin, effGetPlugCategory);

        // if(category == kPlugCategShell) {
        //     add_next_plugin(infos, lib, plugin);
        // } else 
        if (category != kPlugCategOfflineProcess && category != kPlugCategUnknown) {
            infos.push_back(new vst_zzub_info(plugin, path.string(), category));
        }

        return infos;
    }
};




struct vst_plugins : zzub::plugincollection {
    vst_plugins(const char* vst_path);
    virtual void initialize(zzub::pluginfactory* factory);
    virtual const zzub::info* get_info(const char* uri, zzub::archive* data);  // { return 0; }
    virtual const char* get_uri();                                             // { return 0; }
    virtual void configure(const char* key, const char* value);                // {}
    virtual void destroy();

   private:
    const char* vst_path;
};
