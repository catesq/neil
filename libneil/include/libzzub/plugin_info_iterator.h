#pragma once
#include <boost/filesystem.hpp>
#include <string>
#include <algorithm>
#include <type_traits>

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


namespace zzub {
// the vst and vst3 adapters use this to create a zzub::info from a vst
// it is used by the plugin_info_iterator to check a file/directory is a plugin then read all plugins in that file/dir


template<typename T>
struct plugin_info_loader {
    // tests a file/directory to see if it's a plugin (just using file ending for now)  
    virtual bool is_plugin(boost::filesystem::path path) = 0;

    // the get_plugin_infos finds all the plugins in files found by is_plugin
    virtual std::vector<T*> get_plugin_infos(boost::filesystem::path path) = 0;
};



/**
 * @brief The plugin_info_iterator class
 *
 * Rcursively iterate some directories and build zzub::info for every plugin found 
 */

// info_type is a subclass of zzub::info
// info_loader is a subclass of zzub::plugin_info_loader

template<
    typename InfoType, 
    typename LoaderType, 
    typename = std::enable_if_t<std::is_base_of_v<plugin_info_loader<InfoType>, LoaderType>>
>
struct plugin_info_iterator {
    plugin_info_iterator(
        std::string paths, 
        std::string separators=PATH_SEPARATOR
    ) : paths(paths), separators(separators) {
    }


    std::vector<InfoType*> get_plugin_infos() {
        std::vector<InfoType*> plugin_infos {};

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
    // PluginInfoLoader<PluginInfoType> loader = InfoLoader(); 
    LoaderType loader {};
    std::string paths;
    std::string separators = PATH_SEPARATOR;
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

}