#include "VstAdapter.h"
#include "VstPluginInfo.h"
#include "VstDefines.h"
#include <string>
#include <gtk/gtk.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "vstfxstore.h"
#include <bit>
#include <boost/endian/conversion.hpp>
#include <boost/predef/other/endian.h>


//#if GDK_WINDOWING_WIN32

//#include <gdk/win32/gdkwin32.h>
//#define WIN_ID_TYPE HWND
//#define WIN_ID_FUNC(widget) GDK_WINDOW_HWND(gtk_widget_get_window(widget));

//#elif GDK_WINDOWING_QUARTZ

//#include <gdk/quartz/gdkquartz.h>
//#define WIN_ID_TYPE widget
//#define WIN_ID_FUNC(WIDGET) gdk_quartz_window_get_nsview(gtk_widget_get_window(widget));

//#else // GDK_WINDOWING_X11

#include <gdk/gdkx.h>
#define WIN_ID_TYPE gulong
#define WIN_ID_FUNC(widget) gdk_x11_window_get_xid(gtk_widget_get_window(widget))

#define VSTADAPTER(effect) ((VstAdapter*) effect->resvd1)
//#endif


/// TODO proper set/get parameter handling and opcodes: 42,43 & 44 used by oxefm
VstIntPtr VSTCALLBACK hostCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt) {
    // the first call to this function, for opcode audioMasterVersion, is during vst startup
    // and vst_effect->resvd1 is undefined
    if(opcode == audioMasterVersion)
        return 2400;

    assert(effect != nullptr);

    if(!effect->resvd1) {
        return 0;
    }

    VstAdapter *vst_adapter = (VstAdapter*) effect->resvd1;

    switch(opcode) {
    case audioMasterBeginEdit:
        printf("opcode begin edit\n");
        break;

    case audioMasterEndEdit:   // so far I can get the same info from audioMasterAutomate
        printf("opcode end edit\n");
        break;

    case audioMasterAutomate:
    case audioMasterUpdateDisplay:
        printf("\nupdating from ui: index %d, val %.2f\n", index, opt);
        vst_adapter->update_parameter_from_gui(index, opt);
        break;

    case audioMasterProcessEvents: {
        VstEvents* events = (VstEvents*) ptr;
        printf("vst events COUNT: %d\n", events->numEvents);
        break;
    }

    case audioMasterIdle:
        dispatch(effect, effEditIdle);
        break;

    case audioMasterGetTime:
        return (VstIntPtr) vst_adapter->get_vst_time_info();

    case audioMasterGetProductString:
        strncpy((char*) ptr, "Neil modular sequencer", kVstMaxProductStrLen);
        return true;

    case audioMasterGetCurrentProcessLevel:
        return kVstProcessLevelUnknown;

    case DECLARE_VST_DEPRECATED(audioMasterWantMidi):
        return 0;

    default:
        printf("vst hostCallback: missing opcode %d index %d\n", opcode, index);
        break;
    }

    return 0;
}


extern "C" {
    void on_window_destroy(GtkWidget* widget, gpointer data) {
       static_cast<VstAdapter*>(data)->ui_destroy();
    }
}


VstAdapter::VstAdapter(const VstPluginInfo* info) : info(info) {
    for(int idx=0; idx < MAX_TRACKS; idx++) {
        trackvalues[idx].note = zzub::note_value_none;
        trackvalues[idx].volume = VOLUME_DEFAULT;
        trackstates[idx].note = zzub::note_value_none;
        trackstates[idx].volume = VOLUME_DEFAULT;
    }

    globalvals = (uint16_t*) malloc(sizeof(uint16_t) * info->get_param_count());

    track_values = &trackvalues[0];
    global_values = globalvals;

    if(info->flags & zzub_plugin_flag_is_instrument) {
        num_tracks = 1;
        set_track_count(num_tracks);
    }

    memset(&vst_time_info, 0, sizeof(VstTimeInfo));
    vst_events = (VstEvents*) malloc(sizeof(VstEvents) + sizeof(VstEvent*) * (MAX_EVENTS - 2));
    vst_events->numEvents = 0;
    vst_time_info.flags = kVstTempoValid | kVstTransportPlaying;
}


VstAdapter::~VstAdapter() {
    dispatch(plugin, effMainsChanged, 0, 0, NULL, 0.0f);
    dispatch(plugin, effClose);

    clear_vst_events();
    free(globalvals);
    free(vst_events);
}


void VstAdapter::clear_vst_events() {
    for(int idx=0; idx < vst_events->numEvents; idx++) {
        free(vst_events->events[idx]);
        vst_events->events[idx] = nullptr;
    }
    vst_events->numEvents = 0;
}

void VstAdapter::set_track_count(int track_count) {
    if (track_count < num_tracks) {
        for (int t = num_tracks; t < track_count; t++) {
            if (trackstates[t].note != zzub::note_value_none) {
                midi_events.push_back(midi_note_off(trackstates[t].note));
            }
        }
    }

    num_tracks = track_count;
}


VstTimeInfo* VstAdapter::get_vst_time_info() {
    vst_time_info.samplePos   = sample_pos;
    vst_time_info.sampleRate = _master_info->samples_per_second;
    vst_time_info.tempo      = _master_info->beats_per_minute;

    return &vst_time_info;
}


void VstAdapter::init(zzub::archive* pi) {
    plugin = load_vst(lib, info->get_filename(), hostCallback, this);

    if(plugin == nullptr) {
        printf("Unable to load vst: name='%s', file='%s'\n", info->name.c_str(), info->get_filename().c_str());
        return;
    }

    metaplugin = _host->get_metaplugin();
    _host->set_event_handler(metaplugin, this);
    ui_scale = gtk_widget_get_scale_factor((GtkWidget*) _host->get_host_info()->host_ptr);

    dispatch(plugin, effOpen);
    dispatch(plugin, effSetSampleRate, 0, 0, nullptr, (float) _master_info->samples_per_second);
    dispatch(plugin, effSetBlockSize, 0, (VstIntPtr) 256, nullptr, 0);

    printf("plugin %s has %d programs\n", info->name.c_str(), plugin->numPrograms);

    if(plugin->numInputs != 2 && plugin->numInputs > 0)
        audioIn = (float**) malloc(sizeof(float*) * plugin->numInputs );
    else
        audioIn = nullptr;

    if(plugin->numOutputs != 2 && plugin->numOutputs > 0)
        audioOut = (float**) malloc(sizeof(float*) * plugin->numOutputs );
    else
        audioOut = nullptr;

    dispatch(plugin, effMainsChanged, 0, 1, NULL, 0.0f);

    update_zzub_globals_from_plugin(false);
}





// double click in router/open gui - only returns true if a new gui window was opened...
bool VstAdapter::invoke(zzub_event_data_t& data) {
    if (!plugin || data.type != zzub::event_type_double_click || !(info->flags & zzub_plugin_flag_has_custom_gui))
        return true;

    if(is_editor_open)
        return true;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), this);
    gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(_host->get_host_info()->host_ptr));
    gtk_window_set_title(GTK_WINDOW(window), info->name.c_str());
    gtk_window_present(GTK_WINDOW(window));

    auto win_id = WIN_ID_FUNC(window);

    dispatch(plugin, effEditOpen, 0, 0, (void*) win_id, 0);

    ERect* gui_size;
    dispatch(plugin, effEditGetRect, 0, 0, (void*) &gui_size, 0);

    if(gui_size)
        gtk_widget_set_size_request(window, gui_size->right / ui_scale, gui_size->bottom / ui_scale);

    gtk_window_set_resizable (GTK_WINDOW(window), FALSE);
    is_editor_open = true;

    return true;
}


void VstAdapter::ui_destroy() {
    dispatch(plugin, effEditClose);
    is_editor_open = false;
}


void VstAdapter::update_zzub_globals_from_plugin(bool dispatch_control_change) {
//    uint8_t* globals = (uint8_t*) global_values;
    printf("update all params\n");
    for(int idx=0; idx < info->global_parameters.size(); idx++) {
        float vst_val = ((AEffectGetParameterProc)plugin->getParameter)(plugin, idx);

        globalvals[idx] = info->get_vst_param(idx)->vst_to_zzub_value(vst_val);
        if(dispatch_control_change)
//            zzub_plugin_set_parameter_value_direct(metaplugin, 1, 0, idx, globalvals[idx], false);
            _host->control_change(metaplugin, 1, 0, idx, globalvals[idx], false, true);

//        printf("Index: %d, name: %s, vst val: %f, zzub val: %d\n", idx, info->get_vst_param(idx)->zzub_param->name, vst_val, *((uint16_t*)(globals+idx*2)));
    }
}


void VstAdapter::update_parameter_from_gui(int idx, float float_val) {
    printf("update param: index = %d, name: %s,  vst val = %f, zzub val = %d \n", idx, info->get_vst_param(idx)->zzub_param->name, float_val, info->get_vst_param(idx)->vst_to_zzub_value(float_val));
    globalvals[idx] = info->get_vst_param(idx)->vst_to_zzub_value(float_val);
    _host->control_change(metaplugin, 1, 0, idx, globalvals[idx], false, true);
//    zzub_plugin_set_parameter_value_direct(metaplugin, 1, 0, idx, globalvals[idx], false);
}


void VstAdapter::process_events() {
//    uint8_t* globals = (uint8_t*) global_values;
    uint16_t value = 0;

    if(update_zzub_params) {
        update_zzub_params = false;
    }

    for (auto idx = 0; idx < info->get_param_count(); idx++) {
        auto vst_param = info->get_vst_param(idx);


//        switch(vst_param->zzub_param->type) {
//        case zzub::parameter_type_word:
//            value = *((unsigned short*) globals);
//            break;

//        case zzub::parameter_type_switch:
//        case zzub::parameter_type_note:
//        case zzub::parameter_type_byte:
//            value = *((unsigned char*) globals);
//            break;
//        }

//        globals += vst_param->data_size;

        value = globalvals[idx];


        if (value != vst_param->zzub_param->value_none) {
//            printf("process_event: set param. idx: %i, name: '%s', offs=%d, zzubval %d, vst val %f\n", idx, vst_param->zzub_param->name, vst_param->data_offset, value, vst_param->zzub_to_vst_value(value));
            ((AEffectSetParameterProc) plugin->setParameter)(plugin, idx, vst_param->zzub_to_vst_value(value));
        }
    }

    if(info->flags & zzub_plugin_flag_is_instrument)
        process_midi_tracks();
}


void VstAdapter::process_midi_tracks() {
    for(int idx=0; idx < num_tracks; idx++) {
        if (trackvalues[idx].volume != VOLUME_NONE)
            trackstates[idx].volume = trackvalues[idx].volume;


        switch(trackvalues[idx].note) {
        case zzub::note_value_none:
            if(trackstates[idx].note != zzub::note_value_none) {
                if(trackvalues[idx].volume == 0) {
                    midi_events.insert(midi_events.begin(), midi_note_off(trackstates[idx].note));
                    trackstates[idx].note = zzub::note_value_none;
                } else if(trackvalues[idx].volume != VOLUME_NONE) {
                    midi_events.insert(midi_events.begin(), midi_note_aftertouch(trackstates[idx].note, trackstates[idx].volume));
                }
            }

            break;

        case zzub::note_value_off:
            if(trackstates[idx].volume == 0) {
                for(int trk = 0; trk < num_tracks; trk++) {
                    if(trackstates[trk].note != zzub::note_value_none && trackvalues[trk].note != zzub_note_value_none) {
                        midi_events.insert(midi_events.begin(), midi_note_off(trackstates[trk].note));
                        trackstates[trk].note = zzub::note_value_none;
                    }
                }
            } else if(trackstates[idx].note != zzub::note_value_none) {
                midi_events.insert(midi_events.begin(), midi_note_off(trackstates[idx].note));
                trackstates[idx].note = zzub::note_value_none;
            }

            break;

        default:
            if(trackstates[idx].note != zzub::note_value_none) {
                midi_events.insert(midi_events.begin(), midi_note_off(trackstates[idx].note));
            }

            midi_events.push_back(midi_note_on(trackvalues[idx].note, trackstates[idx].volume));
            trackstates[idx].note = trackvalues[idx].note;
            break;
        }

        process_one_midi_track(trackvalues[idx].msg_1, trackstates[idx].msg_1);
        process_one_midi_track(trackvalues[idx].msg_2, trackstates[idx].msg_2);
    }
}


void VstAdapter::process_one_midi_track(midi_msg &vals_msg, midi_msg& state_msg) {
    if(vals_msg.midi.cmd != TRACKVAL_NO_MIDI_CMD) {
        state_msg.midi.cmd = vals_msg.midi.cmd;

        if (vals_msg.midi.data != TRACKVAL_NO_MIDI_DATA)
            state_msg.midi.data = vals_msg.midi.data;

        midi_events.push_back(midi_message(state_msg));
    }
}


bool VstAdapter::process_stereo(float **pin, float **pout, int numsamples, int mode) {
    sample_pos += numsamples;

    if(midi_events.size() > 0) {
        memcpy(&vst_events->events[0], &midi_events[0], midi_events.size() * sizeof(VstMidiEvent*));
        vst_events->numEvents = midi_events.size();
        dispatch(plugin, effProcessEvents, 0, 0, vst_events, 0.f);
        midi_events.clear();
        clear_vst_events();
    }

    if(mode == zzub::process_mode_no_io)
        return 1;

    float **inputs = audioIn ? audioIn : pin;
    float **outputs = audioOut ? audioOut : pout;

    if(audioIn) {
        for(int j=0; j < plugin->numInputs; j++) {
            float *sample = audioIn[0];
            for (int i = 0; i < numsamples; i++)
                *sample++ = (pin[0][i] + pin[1][i]) * 0.5f;
        }
    }

    plugin->processReplacing(plugin, inputs, outputs, numsamples);

    if(audioOut) {
        if(plugin->numOutputs == 1) {
            memcpy(pout[0], audioOut[0], sizeof(float) * numsamples);
            memcpy(pout[1], audioOut[0], sizeof(float) * numsamples);
        } else {
            memcpy(pout[0], audioOut[0], sizeof(float) * numsamples);
            memcpy(pout[1], audioOut[1], sizeof(float) * numsamples);
        }
    }

    return 1;
}

const char *VstAdapter::get_preset_file_extensions() {
    return "Preset file=.fxp:Preset bank=.fxb";
}


bool VstAdapter::save_preset_file(const char* filename) {
    printf("Save preset: %s\n", filename);
    std::filesystem::path path{filename};
    struct fxProgram program{};

    void *chunk_data;
    // ask for the chunk data for a vst Program
    int chunk_size = dispatch(plugin, effGetChunk, 1, 0, &chunk_data, 0);

    //see if effGetChunk worked.
    bool save_chunk = true;
    if(chunk_size == 0) {
        save_chunk = false;
        if(chunk_data) {
            free(chunk_data);
        }
    } else if(!chunk_data) {
        save_chunk = false;
    }

    // if it didn't work then store the params
    if(!save_chunk) {
        chunk_data = malloc(info->get_param_count() * sizeof(float));
        if(!chunk_data)
            goto save_preset_end_false;

        float *curr_param = (float*) chunk_data;
        for(int idx = 0; idx < info->get_param_count(); idx++)
            *curr_param++ = info->get_vst_param(idx)->zzub_to_vst_value(globalvals[idx]);
    }


save_preset_end_true:
    if(chunk_data)
        free(chunk_data);

    return true;


save_preset_end_false:
    return false;
}


bool VstAdapter::load_preset_file(const char* filename) {
    printf("Load preset: %s\n", filename);

    std::filesystem::path path{filename};

    if(!std::filesystem::is_regular_file(path))
        return false;

    auto fsize = std::filesystem::file_size(path);

    if(fsize < sizeof(fxProgram))
       return false;

    std::ifstream file(filename);

    if(!file.is_open())
        return false;

    char* data = (char*) malloc(fsize);

    if(!data)
        return false;

    file.read(data, fsize);

    file.close();

    struct fxProgram* program = (struct fxProgram*) data;
    struct fxBank* bank = (struct fxBank*) data;

    #ifdef BOOST_ENDIAN_LITTLE_BYTE
        boost::endian::big_to_native_inplace(program->chunkMagic);
        boost::endian::big_to_native_inplace(program->fxMagic);
        boost::endian::big_to_native_inplace(program->byteSize);
        boost::endian::big_to_native_inplace(program->version);
        boost::endian::big_to_native_inplace(program->fxID);
        boost::endian::big_to_native_inplace(program->fxVersion);
        boost::endian::big_to_native_inplace(program->numParams);
    #endif

    if(program->chunkMagic != cMagic)
        goto load_preset_end_false;

    if(program->fxID != plugin->uniqueID)
        goto load_preset_end_false;

    switch(program->fxMagic) {
    case fMagic:
        for(int index = 0; index < program->numParams; index++) {
            boost::endian::big_to_native_inplace(program->content.params[index]);
            plugin->setParameter(plugin, index, program->content.params[index]);
        }
        break;

    case chunkPresetMagic:
        boost::endian::big_to_native_inplace(program->content.data.size);
        dispatch(plugin, effSetChunk, 1, program->content.data.size, program->content.data.chunk, 0);
        break;

    case bankMagic:
        break;

    case chunkBankMagic:
        break;
    }

//load_preset_end_true:
    update_zzub_params = true;
    free(data);
    return true;

load_preset_end_false:
    free(data);
    return false;
}

const char *VstAdapter::describe_value(int param, int value) {
    static char cstr[32];
    auto str = std::to_string(value);
    std::strncpy(cstr, str.c_str(), 32);
    return cstr;
}
