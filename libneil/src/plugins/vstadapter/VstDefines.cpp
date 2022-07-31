#include "VstDefines.h"

extern "C" {
    VstIntPtr VSTCALLBACK dummyHostCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt) {
        return 2400;
    }
}




std::string get_param_name(AEffect* plugin, int index) {
    return get_plugin_string(plugin, effGetParamName, index);
}


std::string get_plugin_string(AEffect* plugin, VstInt32 opcode, int index) {
    char vst_chars[64];

    vst_chars[0] = 0;
    dispatch(plugin, opcode, index, 0, (void*) vst_chars, 0.f);
    vst_chars[63]=0;
    std::string vst_string(vst_chars);


    return vst_string;
}


VstParameterProperties* get_param_props(AEffect* plugin, int index) {
    VstParameterProperties* param_props = (VstParameterProperties*) malloc(sizeof(VstParameterProperties));

    if(dispatch(plugin, effGetParameterProperties, index, 0, param_props, 0) == 1)
        return param_props;

    free(param_props);
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

    auto plugin = entryPoint(callback);

    if(!plugin || plugin->magic != kEffectMagic)
        return nullptr;

    plugin->resvd1 =(VstIntPtr) user_p;

    return plugin;
}
