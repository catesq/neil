#include "VstPluginInfo.h"
#include "VstDefines.h"
#include "aeffectx.h"

#include "VstDefines.h"
#include "VstAdapter.h"
#include "VstParameter.h"


VstPluginInfo::VstPluginInfo(AEffect* plugin, std::string filename, VstPlugCategory category) : filename(filename), category(category) {
    version = dispatch(plugin, effGetVendorVersion);

    name = get_plugin_string(plugin, effGetEffectName, kVstMaxEffectNameLen);
    short_name = get_plugin_string(plugin, effGetEffectName, kVstMaxEffectNameLen);
    author = get_plugin_string(plugin, effGetVendorString, kVstMaxVendorStrLen);

    is_synth = plugin->flags & effFlagsIsSynth;

    uri = "@zzub.org/vstadapter/" + name +" /" + std::to_string(version);

    for(int idx=0; idx < plugin->numParams; idx++) {
        param_names.push_back(get_param_name(plugin, idx));
        auto param_properties = get_param_props(plugin, idx);

        auto zzub_param = add_global_parameter().set_word()
                                                .set_name(param_names[idx].c_str())
                                                .set_description(param_names[idx].c_str())
                                                .set_state_flag();

        vst_params.push_back(VstParameter::build(param_properties, &zzub_param));
    }

    if(is_synth) {
        add_track_parameter().set_note();
        add_track_parameter().set_byte()
                             .set_name("Volume")
                             .set_description("Volume")
                             .set_value_min(0)
                             .set_value_max(0x007F)
                             .set_value_none(255)
                             .set_value_default(VOLUME_DEFAULT);
    }

    switch(category) {
        kPlugCategEffect:
        kPlugCategMastering:
        kPlugCategSpacializer:
        kPlugCategRoomFx:
        kPlugSurroundFx:
        kPlugCategRestoration:
        kPlugCategAnalysis:
            flags |= (zzub::plugin_flag_is_effect & zzub::plugin_flag_has_audio_input & zzub::plugin_flag_has_audio_output);
            break;

        kPlugCategSynth:
        kPlugCategGenerator:
            flags |= (zzub::plugin_flag_is_instrument  & zzub::plugin_flag_has_audio_output);
            break;

        default:
            break;
    }
}

bool VstPluginInfo::get_is_synth() const {
    return is_synth;
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


zzub::plugin* VstPluginInfo::create_plugin() const {
    return new VstAdapter(this);
}

