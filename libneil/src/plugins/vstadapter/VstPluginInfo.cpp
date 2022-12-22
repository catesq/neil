#include "VstPluginInfo.h"
#include "VstDefines.h"
#include "aeffectx.h"
#include "aeffect.h"


#include "VstDefines.h"
#include "VstAdapter.h"
#include "VstParameter.h"
#include <string>
#include <iostream>


VstPluginInfo::VstPluginInfo(AEffect* plugin, std::string filename, VstPlugCategory category) : zzub::info(), filename(filename), category(category) {
    dispatch(plugin, effOpen);

    vst_id = plugin->uniqueID;
    version = dispatch(plugin, effGetVendorVersion);
    name = get_plugin_string(plugin, effGetEffectName, 0);
    short_name = get_plugin_string(plugin, effGetEffectName, 0);
    author = get_plugin_string(plugin, effGetVendorString, 0);

    flags |= zzub_plugin_flag_load_presets | zzub_plugin_flag_save_presets;
    if(plugin->flags & effFlagsHasEditor)
        flags |= zzub_plugin_flag_has_custom_gui;

    uri = "@zzub.org/vstadapter/" + name + "/" + std::to_string(version);

    for(int idx=0; idx < plugin->numParams; idx++) {
        auto param_properties = get_param_props(plugin, idx);
        param_names.push_back(get_plugin_string(plugin, effGetParamName, idx));


        int name_str_len = param_names[idx].size();
        char *strcp = (char*) malloc(sizeof(char) * (name_str_len + 1));
        strncpy(strcp, param_names[idx].c_str(), name_str_len);
        strcp[name_str_len] = 0;

        zzub::parameter* zzub_param = &add_global_parameter().set_word()
                                                             .set_name(strcp)
                                                             .set_description(strcp)
                                                             .set_state_flag();

        auto vst_param = VstParameter::build(param_properties, zzub_param, idx);
        vst_params.push_back(vst_param);
        flags  |= zzub_plugin_flag_has_event_input;

        float vst_val = plugin->getParameter(plugin, idx);
        zzub_param->value_default = vst_param->vst_to_zzub_value(vst_val);
    }

    switch(category) {
        case kPlugCategEffect:
        case kPlugCategMastering:
        case kPlugCategSpacializer:
        case kPlugCategRoomFx:
        case kPlugSurroundFx:
        case kPlugCategRestoration:
        case kPlugCategAnalysis:
            flags |= (zzub::plugin_flag_is_effect | zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output);
            break;

        case kPlugCategSynth:
            min_tracks = 1;
            max_tracks = 16;
            flags |= zzub::plugin_flag_has_midi_input;
            add_track_parameter().set_note();
            add_track_parameter().set_byte()
                                 .set_name("Volume")
                                 .set_description("Volume")
                                 .set_value_min(0)
                                 .set_value_max(0x007F)
                                 .set_value_none(VOLUME_NONE)
                                 .set_value_default(VOLUME_DEFAULT);

            add_track_parameter().set_byte()
                                 .set_name("Midi command 1")
                                 .set_description("cmd(0xf0) chan(0x0f)")
                                 .set_value_min(0x80)
                                 .set_value_max(0xfe)
                                 .set_value_none(TRACKVAL_NO_MIDI_CMD)
                                 .set_value_default(TRACKVAL_NO_MIDI_CMD);

            add_track_parameter().set_word()
                                 .set_name("Midi data 1")
                                 .set_description("byte1(0x7f00) byte2(0x007f)")
                                 .set_value_min(0)
                                 .set_value_max(0xfffe)
                                 .set_value_none(0xffff)
                                 .set_value_default(0);

            add_track_parameter().set_byte()
                                 .set_name("Midi command 2")
                                 .set_description("status(0xf0) chan(0x0f)")
                                 .set_value_min(0x80)
                                 .set_value_max(0xfe)
                                 .set_value_none(TRACKVAL_NO_MIDI_CMD)
                                 .set_value_default(TRACKVAL_NO_MIDI_CMD);

            add_track_parameter().set_word()
                                 .set_name("Midi data 2")
                                 .set_description("byte1(0x7f00) byte2(0x007f)")
                                 .set_value_min(0)
                                 .set_value_max(0xfffe)
                                 .set_value_none(0xffff)
                                 .set_value_default(0);

        case kPlugCategGenerator:
            flags |= (zzub::plugin_flag_is_instrument | zzub::plugin_flag_has_audio_output | zzub::plugin_flag_is_instrument);
            break;

        case kPlugCategUnknown:
        default:
                printf("vst: Unknown plugin category %d for %s\n", category, name.c_str());
            break;
    }

    dispatch(plugin, effClose);
}

/// TODO make a move constructor and a move constructor - to reuse/free the malloc'd strings in the zuub::param in global_parameters and reallocte
/// the plugininfo will be reused and only be destroyed when the program is closed so putting this off is unclean but irrelevant
VstPluginInfo::~VstPluginInfo() {
}

bool VstPluginInfo::get_is_synth() const {
    return flags & zzub::plugin_flag_is_instrument;
}

std::string VstPluginInfo::get_filename() const {
    return filename;
}

int VstPluginInfo::get_param_count() const {
    return global_parameters.size();
}


const std::vector<std::string>& VstPluginInfo::get_param_names() const {
    return param_names;
}

const std::vector<VstParameter*>& VstPluginInfo::get_vst_params() const {
    return vst_params;
}

VstParameter* VstPluginInfo::get_vst_param(int index) const {
    return vst_params[index];
}

zzub::plugin* VstPluginInfo::create_plugin() const {
    return new VstAdapter(this);
}

