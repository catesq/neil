#pragma once
#include <boost/filesystem.hpp>
#include <string>
#include <algorithm>

#if defined(WINOS)
#define strtok_r strtok_s
#endif

#if defined(MACOS)
#define PATH_SEPARATOR ";"
#elif defined(WINOS)
#define PATH_SEPARATOR ":"
#else
#define PATH_SEPARATOR ":"
#endif



// T will be a subclass of zzub::info used as an adapter between lv2/vst plugins and zzub plugins 
// eg Vst3Info from libneils/src/plugins/vst3adapter/Info.h

// a PluginInfoLoader is used by the PluginInfoIterator

template<typename T>
struct PluginInfoLoader {
    // tests a file/directory to see if it's a plugin (just using file ending for now)  
    virtual bool is_plugin(boost::filesystem::path path) = 0;

    // the get_plugin_infos finds all the plugins in files found by is_plugin
    virtual std::vector<T*> get_plugin_infos(boost::filesystem::path path) = 0;
};


/**
 * @brief The PluginInfoIterator class
 *
 * Iterate over all files in a list of directories, check if each file is a vst/lv2/lv2 plugin
 * and build zzub::info a object for every plugin found 
 * 
 */


template<typename PluginInfoType>
struct PluginInfoIterator {
    /**
     * Example:
     *
     * struct PluginInfo {};
     *
     * void add_plugin(PluginInfoIterator<PluginInfo>& infos, boost::filesystem::path filename) {
     *     if(is_valid_plugin(filename)) {
     *         infos.add_info(new Vst3PluginInfo{});
     *     }
     * }
     *
     * for(PluginInfo* plugin_info: PluginInfoIterator("/path/to/vst/:/separated/by/")) {
     *     // this is used in zzub::plugincollection::initialize(zzub::pluginfactory* factory)
     *     // and factory->register_info(plugin_info); is the only use
     * }
     */

    PluginInfoIterator(
        PluginInfoLoader<PluginInfoType>& loader, 
        std::string paths, 
        std::string separators=PATH_SEPARATOR
    ) : loader(loader), paths(paths), separators(separators) {
    }


    std::vector<PluginInfoType*> get_plugin_infos() {
        std::vector<PluginInfoType*> plugin_infos {};

        auto directory_names = get_directory_names((char*)paths.c_str(), separators.c_str());

        for(auto& dir: directory_names) {
            if(boost::filesystem::is_directory(dir)) {
                auto dir_iter = boost::filesystem::recursive_directory_iterator(dir);
                
                for(auto& entry: dir_iter) {
                    auto path = weakly_canonical(entry.path());

                    if(std::find(checked_paths.begin(), checked_paths.end(), path.string()) != checked_paths.end()) {
                        continue;
                    }

                    checked_paths.push_back(path.string());

                    if(loader.is_plugin(path)) {
                        for(auto& plugin_info: loader.get_plugin_infos(path)) {
                            plugin_infos.push_back(plugin_info);
                        }
                    }
                }
            }
        }

        return plugin_infos;
    }


private:
    PluginInfoLoader<PluginInfoType>& loader; 
    std::string paths;
    std::string separators=PATH_SEPARATOR;
    std::vector<std::string> checked_paths;
    

    // split a text separated by ';' and maybe other charecters into a vector of directory names
    std::vector<std::string> get_directory_names(char* path_str, const char* separators) {
        std::vector<std::string> dirs;

        char* path_str_curr_p = NULL;
        char* dir;

        for(dir = strtok_r(path_str, separators, &path_str_curr_p);
            dir != NULL;
            dir = strtok_r(NULL, separators, &path_str_curr_p)
        ) {
            dirs.push_back(std::string(dir));
        }

        return dirs;
    }

};
