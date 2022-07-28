#include "VstDefines.h"

extern "C" {
    VstIntPtr VSTCALLBACK dummyHostCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt) {
        return 2400;
    }
}

std::string get_plugin_name(AEffect* plugin) {
    char name[34];
    dispatch(plugin, effGetEffectName, 0, &name[0]);
    name[33]=0;

    return name;
}

std::string get_param_name(AEffect* plugin, int index) {
    char name[10];
    dispatch(plugin, effGetParamName, index, &name[0]);
    name[9]=0;

    return name;
}


VstParameterProperties* get_param_props(AEffect* plugin, int index) {
    VstParameterProperties* param_props = nullptr;

    if(dispatch(plugin, effGetParameterProperties, 0, &param_props) == 1)
        return param_props;
    else
        return nullptr;
}


AEffect* load_vst(boost::dll::shared_library& lib, std::string vst_filename, AEffectDispatcherProc callback, void* user_p) {
    boost::system::error_code ec{};
    lib.load(vst_filename, ec);

    if(ec)
        return nullptr;

    auto entryPoint = lib.get<AEffect*(AEffectDispatcherProc)>("VSTPluginMain");

    if(!entryPoint)
        return nullptr;

    auto plugin = entryPoint(dummyHostCallback);

    if(!plugin || plugin->magic != kEffectMagic)
        return nullptr;

    plugin->user = user_p;

    return plugin;
}
