#include <string>
#include <iostream>

#include "aeffectx.h"
#include "aeffect.h"

#include "vst_plugin_info.h"
#include "vst_defines.h"
#include "vst_parameter.h"
#include "vst_adapter.h"


vst_zzub_info::vst_zzub_info(AEffect* plugin, 
                             std::string filename, 
                             VstPlugCategory category) 
    : zzub::info(), filename(filename), category(category) 
{
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

        auto vst_param = vst_parameter::build(param_properties, zzub_param, idx);
        vst_params.push_back(vst_param);
        flags  |= zzub_plugin_flag_has_event_input;

        float vst_val = plugin->getParameter(plugin, idx);
        zzub_param->value_default = vst_param->vst_to_zzub_value(vst_val);
    }

    add_attribute()
			.set_name("MIDI channel (0 = disabled, 17 = omni)")
			.set_value_min(0)
			.set_value_max(17)
			.set_value_default(0);

    add_attribute()
			.set_name("Auto midi note off. if note playing and note length not set for that note")
			.set_value_min(0)
			.set_value_max(1)
			.set_value_default(0);

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

            zzub::midi_track_manager::add_midi_track_info(this);

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
vst_zzub_info::~vst_zzub_info() {
}


bool 
vst_zzub_info::get_is_synth() const 
{
    return flags & zzub::plugin_flag_is_instrument;
}


std::string 
vst_zzub_info::get_filename() const 
{
    return filename;
}


int 
vst_zzub_info::get_param_count() const 
{
    return global_parameters.size();
}


const std::vector<std::string>& 
vst_zzub_info::get_param_names() const 
{
    return param_names;
}


const std::vector<vst_parameter*>& 
vst_zzub_info::get_vst_params() const 
{
    return vst_params;
}


vst_parameter* 
vst_zzub_info::get_vst_param(int index) const 
{
    return vst_params[index];
}


zzub::plugin* 
vst_zzub_info::create_plugin() const {
    return new vst_adapter(this);
}

