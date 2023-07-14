#pragma once

#include <cstdlib>
#include <iostream>
#include <map>

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

        if(modules.find(path.string()) != modules.end()) {
            return infos;
        }

        std::string error_str;
        auto _module = VST3::Hosting::Module::create(path.string(), error_str);

        if (!_module) {
            std::cerr << "Error loading VST3 plugin \"" << path << "\": " << error_str << std::endl;
            return infos;
        }

        modules.insert(std::make_pair(path.string(), _module));

        const VST3::Hosting::PluginFactory& factory = _module->getFactory();
        for (auto class_info : factory.classInfos()) {
            if (class_info.category() != kVstAudioEffectClass) {
                continue;
            }

            auto provider = new Steinberg::Vst::PlugProvider(factory, class_info, true);

            if (!provider) {
                std::cerr << "No plugin provider found for \"" << class_info.name() << " from " << path << std::endl;
				continue;
			}
    
            auto info = new Vst3Info(path.string(), _module, class_info, provider);
            delete provider;

            if (info->is_valid()) {
                infos.push_back(info);
            } else {
                delete info;
            }
        }

        return infos;
    }
private:
    std::map<std::string, VST3::Hosting::Module::Ptr> modules;
};
