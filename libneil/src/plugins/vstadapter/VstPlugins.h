#pragma once
#include "string.h"

#include "zzub/plugin.h"
#include "zzub/signature.h"
#include <boost/filesystem.hpp>
#include <boost/dll.hpp>

#include "aeffect.h"
#include "aeffectx.h"

#if defined(_MSC_VER)
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

extern "C" {
    typedef VstIntPtr(*HostCallbackFunc)(AEffect *effect, VstInt32 opcode, VstInt32 index, VstInt32 value, void *ptr, float opt);
    VstIntPtr VSTCALLBACK dummyHostCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstInt32 value, void *ptr, float opt);
}

struct VstDescription {
    VstDescription(std::string name, std::string filename, bool is_synth) : name(name), filename(filename), isSynth(is_synth) {}

    std::string name;
    std::string filename;
    bool isSynth;
};

struct VstDescriptions {
    VstDescriptions(const char* path_str) {
        read_vsts(path_str);
    }


    std::vector<VstDescription>::iterator begin() {
        return vsts.begin();
    }

    std::vector<VstDescription>::iterator end() {
        return vsts.end();
    }

private:
    void read_vsts(const char* path_str) {
        for(auto& dir: get_dirs((char*) path_str)){
            read_vst_dir(dir);
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
        for(auto& entry: boost::filesystem::directory_iterator(dir)) {
            try_vst_load(entry.path());
        }
    }

    void try_vst_load(boost::filesystem::path path) {
        if(!boost::filesystem::is_regular_file(path))
            return;

        if(path.extension().string() != VST_EXT)
            return;

        boost::system::error_code ec{};
        boost::dll::shared_library lib(path.string(), ec);

        if(ec)
            return;

        auto entryPoint = lib.get<AEffect*(HostCallbackFunc)>("VSTPluginMain");

        if(!entryPoint)
            return;

        typedef VstIntPtr (*dispatcherFuncPtr)(AEffect *effect, VstInt32 opCode, VstInt32 index, VstInt32 value, void *ptr, float opt);
        AEffect* plugin = entryPoint(dummyHostCallback);

        char vst_name[34];
        dispatcherFuncPtr dispatcher = (dispatcherFuncPtr) plugin->dispatcher;
        dispatcher(plugin, effGetEffectName, 0, 0, &vst_name[0], 0);
        vst_name[33]=0;

        bool issynth = plugin->flags & effFlagsIsSynth;

        vsts.emplace_back(vst_name, path.string(), issynth);
    }

    std::vector<VstDescription> vsts;
};



struct VstPlugins : zzub::plugincollection {
    VstPlugins(const char* vst_path);
    virtual void initialize(zzub::pluginfactory *factory);
    virtual const zzub::info *get_info(const char *uri, zzub::archive *data); // { return 0; }
    virtual const char *get_uri(); // { return 0; }
    virtual void configure(const char *key, const char *value); // {}
    virtual void destroy();
private:
    const char* vst_path;
};





