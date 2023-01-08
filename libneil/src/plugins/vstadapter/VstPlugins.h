#pragma once
#include "string.h"

#include "zzub/plugin.h"
#include "zzub/signature.h"
#include <boost/filesystem.hpp>


#include "aeffect.h"

#include "VstPluginInfo.h"
#include "VstDefines.h"

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




struct vst_plugin_file_finder {
    vst_plugin_file_finder(const char* path_str) {
        read_vsts(path_str);
    }

    std::vector<vst_zzub_info*>::iterator begin() {
        return vsts.begin();
    }

    std::vector<vst_zzub_info*>::iterator end() {
        return vsts.end();
    }

private:
    void read_vsts(const char* path_str) {
        if(boost::filesystem::is_directory(path_str)) {
            for(auto& dir: get_dirs((char*) path_str))
                read_vst_dir(dir);
        } else {
            printf("vst adapter vst search dir '%s' was not found\n", path_str);
        }
    }

    std::vector<std::string> get_dirs(char* path_str) {
        std::vector<std::string> dirs;

        char* path_str_curr_p = NULL;
        const char* separators = ":";
        char* dir;

        for( dir = strtok_r(path_str, separators, &path_str_curr_p);
             dir != NULL;
             dir = strtok_r(NULL, separators, &path_str_curr_p)
           ) {
            dirs.push_back(std::string(dir));
        }

        return dirs;
    }

    void read_vst_dir(std::string dir) {
        for(auto& entry: boost::filesystem::recursive_directory_iterator(dir)) {
            try_vst_load(entry.path());
        }
    }

    void try_vst_load(boost::filesystem::path path) {
        if(!boost::filesystem::is_regular_file(path))
            return;

        if(path.extension().string() != VST_EXT)
            return;

        boost::dll::shared_library lib{};
        AEffect* plugin = load_vst(lib, path.string(), dummyHostCallback, nullptr);

        if(plugin == nullptr)
            return;

        VstPlugCategory category = (VstPlugCategory) dispatch(plugin, effGetPlugCategory);

        if(category == kPlugCategOfflineProcess  || category == kPlugCategUnknown)
            return;

        if(category == kPlugCategShell)
            add_next_plugin(lib, plugin);
        else
            vsts.push_back(new vst_zzub_info(plugin, path.string(), category));

    }

    void add_next_plugin(boost::dll::shared_library& lib, AEffect* plugin) {

    }

    std::vector<vst_zzub_info*> vsts;
};



struct vst_plugins : zzub::plugincollection {
    vst_plugins(const char* vst_path);
    virtual void initialize(zzub::pluginfactory *factory);
    virtual const zzub::info *get_info(const char *uri, zzub::archive *data); // { return 0; }
    virtual const char *get_uri(); // { return 0; }
    virtual void configure(const char *key, const char *value); // {}
    virtual void destroy();
private:
    const char* vst_path;
};





