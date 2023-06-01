#pragma once

#include <cstdlib>
#include <iostream>

#include "boost/filesystem.hpp"

#include "plugin_info_iterator.h"

#include "Collection.h"
#include "Info.h"


struct Vst3InfoLoader: public PluginInfoLoader<struct Vst3Info> {
    virtual bool is_plugin(boost::filesystem::path path) override {
        return boost::filesystem::is_directory(path) && path.extension().string() == ".vst3";
    }

    virtual std::vector<struct Vst3Info*> get_plugin_infos(boost::filesystem::path path) override {
        auto infos = std::vector<struct Vst3Info*> {};
        std::string error_str;
        auto _module = VST3::Hosting::Module::create(path.string(), error_str);

        if (!_module) {
            std::cerr << "Error loading VST3 plugin \"" << path << "\": " << error_str << std::endl;
            return infos;
        }

        const VST3::Hosting::PluginFactory& factory = _module->getFactory();
        for (auto &class_info : factory.classInfos()) {
            if (class_info.category() != kVstAudioEffectClass) {
                continue;
            }

            auto info = new Vst3Info(path.string(), factory, class_info);

            if (info->is_valid()) {
                infos.push_back(info);
            } else {
                delete info;
            }
        }

        return infos;
    }
};
