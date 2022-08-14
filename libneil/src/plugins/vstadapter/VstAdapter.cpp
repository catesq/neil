#include "VstAdapter.h"
#include "VstPluginInfo.h"
#include "VstDefines.h"
#include <string>
#include <gtk/gtk.h>


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
    if(opcode == audioMasterVersion)
        return 2400;

    assert(effect != nullptr);

    // avoid a segfault from unloaded plugins using this callback - with opcode audioMasterVersion - when vst_adapter is null
    VstAdapter *vst_adapter = (VstAdapter*) effect->resvd1;

    switch(opcode) {
    case audioMasterBeginEdit:
    case audioMasterEndEdit:   // so far I can get the same info from audioMasterAutomate
        break;

    case audioMasterAutomate:
        vst_adapter->update_parameter_from_gui(index, opt);
        break;

    case audioMasterIdle:
        dispatch(effect, effEditIdle);
        break;

    case audioMasterGetTime:
        return (VstIntPtr) vst_adapter->get_vst_time_info(true);

    case audioMasterGetCurrentProcessLevel:
        return kVstProcessLevelUnknown;

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

    delete globalvals;
    clear_vst_events();
    free(vst_events);
}

void VstAdapter::clear_vst_events() {
    for(int idx=0; idx < vst_events->numEvents; idx++) {
        free(vst_events->events[idx]);
        vst_events->events[idx] = nullptr;
    }
    vst_events->numEvents = 0;
}


VstTimeInfo* VstAdapter::get_vst_time_info(bool update) {
    if(!update)
        return &vst_time_info;

    vst_time_info.samplePos   = sample_pos;
    vst_time_info.sampleRate = _master_info->samples_per_second;
    vst_time_info.tempo      = _master_info->beats_per_minute;

    return &vst_time_info;
}


void VstAdapter::init(zzub::archive* pi) {
    plugin = load_vst(lib, info->get_filename(), hostCallback, this);

    if(plugin == nullptr)
        return;

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

    _host->set_event_handler(_host->get_metaplugin(), this);
    dispatch(plugin, effMainsChanged, 0, 1, NULL, 0.0f);


    for(int idx=0; idx < info->global_parameters.size(); idx++) {
        float vst_val = ((AEffectGetParameterProc)plugin->getParameter)(plugin, idx);
        globalvals[idx] = info->get_vst_param(idx)->vst_to_zzub_value(vst_val);
    }
}




bool VstAdapter::invoke(zzub_event_data_t& data) {
    if (!plugin || data.type != zzub::event_type_double_click || !(info->flags & zzub_plugin_flag_has_custom_gui))
        return false;

    if(is_editor_open)
        return true ;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), this);

    gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(_host->get_host_info()->host_ptr));
    gtk_window_set_title(GTK_WINDOW(window), info->name.c_str());
    gtk_window_present(GTK_WINDOW(window));

    auto win_id = WIN_ID_FUNC(window);

    dispatch(plugin, effEditOpen, 0, 0, (void*) win_id, 0);

    ERect* gui_size;
    dispatch(plugin, effEditGetRect, 0, 0, (void*) &gui_size, 0);

    if(gui_size != 0)
        gtk_widget_set_size_request(window, gui_size->right, gui_size->bottom);

    gtk_window_set_resizable (GTK_WINDOW(window), FALSE);

    is_editor_open = true;

    return true;
}

void VstAdapter::ui_destroy() {
    dispatch(plugin, effEditClose);
    is_editor_open = false;
}

void VstAdapter::update_parameter_from_gui(int index, float float_val) {
    globalvals[index] = info->get_vst_param(index)->vst_to_zzub_value(float_val);
}




void VstAdapter::process_events() {
    uint8_t* globals = (uint8_t*) global_values;
    uint16_t value = 0;

    auto& params = info->get_vst_params();
    for (auto idx = 0; idx < params.size(); idx++) {
        auto vst_param = params[idx];

        switch(vst_param->zzub_param->type) {
        case zzub::parameter_type_word:
            value = *((unsigned short*) globals);
            break;

        case zzub::parameter_type_switch:
        case zzub::parameter_type_note:
        case zzub::parameter_type_byte:
            value = *((unsigned char*) globals);
            break;
        }

        globals += vst_param->data_size;

        if (value != vst_param->zzub_param->value_none) {
            printf("set param. idx: %i, name: '%s', zzubval %d, vst val %f\n", idx, vst_param->zzub_param->name, value, vst_param->zzub_to_vst_value(value));
            ((AEffectSetParameterProc) plugin->setParameter)(plugin, idx, vst_param->zzub_to_vst_value(value));
        }
    }

    if(!(info->flags & zzub_plugin_flag_is_instrument))
        return;

    for(int idx=0; idx < num_tracks; idx++) {
        if (trackvalues[idx].volume != VOLUME_NONE)
            trackstates[idx].volume = trackvalues[idx].volume;

        if (trackvalues[idx].note == zzub::note_value_none) {

            if(trackvalues[idx].volume != VOLUME_NONE && trackstates[idx].note != zzub::note_value_none) {
                midi_events.push_back(midi_note_aftertouch(trackstates[idx].note, trackstates[idx].volume));
            }

        } else if(trackvalues[idx].note != zzub::note_value_off) {

            midi_events.push_back(midi_note_on(trackvalues[idx].note, trackstates[idx].volume));
            trackstates[idx].note = trackvalues[idx].note;

        } else if(trackstates[idx].note != zzub::note_value_none) {

            midi_events.push_back(midi_note_off(trackstates[idx].note));

            // this is wrong but some synths glitch when an aftertouch is sent after a note off
            // it relies on state.note being a valid note.
            // if a note off with volume 0 is set then clear state.note to prevent aftertouch
            if(trackvalues[idx].volume == 0)
                trackstates[idx].note = zzub::note_value_none;
        }
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

const char *VstAdapter::describe_value(int param, int value) {
    static char cstr[32];
    auto str = std::to_string(value);
    std::strncpy(cstr, str.c_str(), 32);
    return cstr;
}
