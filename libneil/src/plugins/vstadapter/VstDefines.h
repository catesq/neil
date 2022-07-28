#pragma once

#include "aeffect.h"
#include "aeffectx.h"
#include <string>
#include <boost/dll.hpp>

#define VOLUME_DEFAULT 0x40

std::string get_plugin_name(AEffect* plugin);
std::string get_param_name(AEffect* plugin, int index);
VstParameterProperties* get_param_props(AEffect* plugin, int index);
AEffect* load_vst(boost::dll::shared_library& lib, std::string vst_filename, AEffectDispatcherProc callback, void* user_p);

extern "C" {
    // only used by VstPlugins class when searching for plugins in the vst folders. plugins tend to ask what version of vst the host supports (opcode 1).
    VstIntPtr VSTCALLBACK dummyHostCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
}


inline VstInt32 dispatch(AEffect* plugin, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt) {
    return ((AEffectDispatcherProc) plugin->dispatcher)(plugin, opcode, index, value, ptr, opt);
}


inline VstInt32 dispatch(AEffect* plugin, VstInt32 opcode, VstInt32 index, void *ptr) {
    return dispatch(plugin, opcode, index, 0, ptr, 0);
}

inline VstInt32 dispatch(AEffect* plugin, VstInt32 opcode, void *ptr) {
    return dispatch(plugin, opcode, 0, 0, ptr, 0);
}

inline VstInt32 dispatch(AEffect* plugin, VstInt32 opcode, VstIntPtr value) {
    return dispatch(plugin, opcode, 0, value, nullptr, 0);
}

inline VstInt32 dispatch(AEffect* plugin, VstInt32 opcode, float value) {
    return dispatch(plugin, opcode, 0, 0, nullptr, value);
}

inline VstInt32 dispatch(AEffect* plugin, VstInt32 opcode) {
    return dispatch(plugin, opcode, 0, 0, 0, 0);
}
