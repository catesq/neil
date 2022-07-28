#include "VstDefines.h"

extern "C" {
    VstIntPtr VSTCALLBACK dummyHostCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt) {
        return 2400;
    }
}




std::string get_param_name(AEffect* plugin, int index) {
    return get_plugin_string(plugin, effGetParamName, kVstMaxParamStrLen, index);
}


std::string get_plugin_string(AEffect* plugin, VstInt32 opcode, unsigned maxlen, int index) {
    char* vst_char_p= (char*) malloc(maxlen+1);
    vst_char_p[maxlen]=0;

    dispatch(plugin, opcode, index, vst_char_p);

    std::string vst_string(vst_char_p);
    free(vst_char_p);

    return vst_string;
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
