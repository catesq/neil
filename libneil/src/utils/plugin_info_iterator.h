#pragma once
#include <boost/filesystem.hpp>
#include <string>

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

/**
 * @brief The PluginInfoIterator class
 *
 * Recursively iterate through some plugin directories, to find any files
 * with a filename extension equal to [".so" or ".dll" or ".vst" - depending on platform]

 * Matching filenames are passed to the maybe_add_plugin_info(info_iterator, path) callback

 * the callback function checks it's a valid plugin and calls info_iterator.add_info on any valid plugin(s)
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
    using ThisType = PluginInfoIterator<PluginInfoType>;
    using AddInfo = std::function<void(ThisType& self, boost::filesystem::path)>;
    using CanAdd = std::function<bool(boost::filesystem::path)>;

    PluginInfoIterator(CanAdd can_add, AddInfo add_info, std::string paths, std::string separators=PATH_SEPARATOR) {
        for(auto& dir: get_dirs((char*)paths.c_str(), separators.c_str()))
            if(boost::filesystem::is_directory(dir))
                for(auto& entry: boost::filesystem::recursive_directory_iterator(dir, boost::filesystem::symlink_option::recurse))
                    if(can_add(entry.path()))
                        add_info(*this, entry.path());
    }

    typename std::vector<PluginInfoType*>::iterator begin() {
        return plugin_infos.begin();
    }

    typename std::vector<PluginInfoType*>::iterator end() {
        return plugin_infos.end();
    }

    void add(PluginInfoType* info) {
        plugin_infos.push_back(info);
    }

private:

    void maybe_add(const boost::filesystem::path& path, CanAdd can_add) {
        if(already_iterated_dir(path))
            return;

        auto add_plugin_info = can_add(*this, path);

        if(!add_plugin_info)
            return;

        add_plugin_info(*this, path);
    }


    bool already_iterated_dir(const boost::filesystem::path& path) {
        auto dir = boost::filesystem::canonical(path.parent_path());
        for(auto& iterated_file: already_done)
            if(dir.string().find(iterated_file) != std::string::npos)
                return true;

        return false;
    }

    std::vector<std::string> already_done;

    std::vector<std::string> get_dirs(char* path_str, const char* separators) {
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

    std::vector<PluginInfoType*> plugin_infos;
};
