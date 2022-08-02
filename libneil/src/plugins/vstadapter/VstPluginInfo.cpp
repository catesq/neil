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

    if(plugin->flags & effFlagsHasEditor)
        flags |= zzub_plugin_flag_has_custom_gui;

    uri = "@zzub.org/vstadapter/" + name + "/" + std::to_string(version);

    uint16_t offset = 0;
    for(int idx=0; idx < plugin->numParams; idx++) {
        auto param_properties = get_param_props(plugin, idx);
        param_names.push_back(get_plugin_string(plugin, effGetParamName, idx));

        add_global_parameter().set_word()
                              .set_name(param_names[idx].c_str())
                              .set_description(param_names[idx].c_str())
                              .set_state_flag();

        vst_params.push_back(VstParameter::build(param_properties, global_parameters[idx], offset));
        offset += vst_params[idx]->data_size;
    }

    switch(category) {
        case kPlugCategEffect:
        case kPlugCategMastering:
        case kPlugCategSpacializer:
        case kPlugCategRoomFx:
        case kPlugSurroundFx:
        case kPlugCategRestoration:
        case kPlugCategAnalysis:
            flags |= (zzub::plugin_flag_is_effect | zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output | zzub::plugin_flag_is_effect);
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

        case kPlugCategGenerator:
            flags |= (zzub::plugin_flag_is_instrument | zzub::plugin_flag_has_audio_output | zzub::plugin_flag_is_instrument);
            break;

        case kPlugCategUnknown:
        default:
                printf("vst: Unknown plugin category %d for %s\n", category, name.c_str());
            break;

        dispatch(plugin, effClose);
    }
}

bool VstPluginInfo::get_is_synth() const {
    return flags & zzub::plugin_flag_is_instrument;
}

std::string VstPluginInfo::get_filename() const {
    return filename;
}

int VstPluginInfo::get_param_count() const {
    return param_names.size();
}


const std::vector<std::string>& VstPluginInfo::get_param_names() const {
    return param_names;
}


const std::vector<VstParameter*>& VstPluginInfo::get_vst_params() const {
    return vst_params;
}

VstParameter* VstPluginInfo::get_vst_param(int index) const {
    return vst_params.at(index);
}

zzub::plugin* VstPluginInfo::create_plugin() const {
    return new VstAdapter(this);
}

