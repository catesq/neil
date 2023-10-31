#pragma once 

#include <string>
#include <vector>

#include "zzub/parameter.h"

namespace zzub {

struct lib;
struct plugin;
struct archive;

struct info {
    int version;
    int flags;
    unsigned int min_tracks;
    unsigned int max_tracks;
    std::string name;
    std::string short_name;
    std::string author;
    std::string commands;
    lib *plugin_lib;
    std::string uri;

    std::vector<const zzub::parameter *> global_parameters;

    std::vector<const zzub::parameter *> track_parameters;

    // for controller plugins: those will be associated with parameters of remote plugins
    // they are purely internal and will not be visible in the pattern editor or gui
    std::vector<const zzub::parameter *> controller_parameters;

    std::vector<const zzub::attribute *> attributes;

    // details about what formats import and stream plugins handle
    std::vector<std::string> supported_import_extensions;
    std::vector<std::string> supported_stream_extensions;

    virtual zzub::plugin *create_plugin() const = 0;
    virtual bool store_info(zzub::archive *arc) const = 0;

    zzub::parameter &add_global_parameter() {
        zzub::parameter *param = new zzub::parameter();
        global_parameters.push_back(param);
        return *param;
    }

    zzub::parameter &add_track_parameter() {
        zzub::parameter *param = new zzub::parameter();
        track_parameters.push_back(param);
        return *param;
    }

    zzub::parameter &add_controller_parameter() {
        zzub::parameter *param = new zzub::parameter();
        controller_parameters.push_back(param);
        return *param;
    }

    zzub::attribute &add_attribute() {
        zzub::attribute *attrib = new zzub::attribute();
        attributes.push_back(attrib);
        return *attrib;
    }

    info() {
        version = zzub::version;
        flags = 0;
        min_tracks = 0;
        max_tracks = 0;
        name = "";
        short_name = "";
        author = "";
        commands = "";
        plugin_lib = 0;
        uri = "";
    }

    virtual ~info() {
        for (std::vector<const zzub::parameter *>::iterator i = global_parameters.begin();
             i != global_parameters.end(); ++i) {
            delete *i;
        }
        global_parameters.clear();
        for (std::vector<const zzub::parameter *>::iterator i = track_parameters.begin();
             i != track_parameters.end(); ++i) {
            delete *i;
        }
        track_parameters.clear();
        for (std::vector<const zzub::parameter *>::iterator i = controller_parameters.begin();
             i != controller_parameters.end(); ++i) {
            delete *i;
        }
        controller_parameters.clear();
        for (std::vector<const zzub::attribute *>::iterator i = attributes.begin();
             i != attributes.end(); ++i) {
            delete *i;
        }
        attributes.clear();
    }

    static int calc_column_size(const std::vector<const zzub::parameter *> &params) {
        int size = 0;
        for (unsigned i = 0; i < params.size(); i++) {
            size += params[i]->get_bytesize();
        }
        return size;
    }

    int get_group_size(int group) const {
        switch (group) {
            case 0:
                return 2 * sizeof(short);
            case 1:
                return calc_column_size(global_parameters);
            case 2:
                return calc_column_size(track_parameters);
            default:
                return 0;
        }
    }
};

}