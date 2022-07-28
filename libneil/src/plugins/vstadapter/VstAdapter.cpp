#include "VstAdapter.h"
#include "VstPluginInfo.h"
#include "VstDefines.h"
#include <string>
#include <gtk/gtk.h>


#if GDK_WINDOWING_WIN32

#define WIN_ID_TYPE HWND
#define WIN_ID_FUNC(widget) GDK_WINDOW_HWND(gtk_widget_get_window(widget));
#include <gdk/win32/gdkwin32.h>

#elif GDK_WINDOWING_QUARTZ

#define WIN_ID_TYPE widget
#define WIN_ID_FUNC(WIDGET) gdk_quartz_window_get_nsview(gtk_widget_get_window(widget));
#include <gdk/quartz/gdkquartz.h>

#else // GDK_WINDOWING_X11

#define WIN_ID_TYPE gulong
#define WIN_ID_FUNC(widget) GDK_WINDOW_XID(gtk_widget_get_window(widget))
#include <gdk/gdkx.h>

#endif

VstIntPtr VSTCALLBACK hostCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt) {
    switch(opcode) {
    case audioMasterVersion:
        return 2400;
    case audioMasterIdle:
        dispatch(effect, effEditIdle);
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
    for(auto& trackval: trackvals) {
        trackval.note = zzub::note_value_none;
        trackval.volume = VOLUME_DEFAULT;
    }

    globalvals = (uint16_t*) malloc(sizeof(uint16_t) * info->get_param_count());

    track_values = &trackvals[0];
    global_values = globalvals;

    if(info->get_is_synth())
        num_tracks = 1;
}

VstAdapter::~VstAdapter() {
    delete globalvals;
}

void VstAdapter::init(zzub::archive* pi) {
    plugin = load_vst(lib, info->get_filename(), hostCallback, this);

    if(plugin == nullptr)
        return;

    dispatch(plugin, effOpen);
    dispatch(plugin, effSetSampleRate, (float) _master_info->samples_per_second);
    dispatch(plugin, effSetBlockSize, (VstIntPtr) 256);

    if(plugin->numInputs != 2 && plugin->numInputs > 0)
        audioIn = (float**) malloc(sizeof(float*) * plugin->numInputs );
    else
        audioIn = nullptr;

    if(plugin->numOutputs != 2 && plugin->numOutputs > 0)
        audioOut = (float**) malloc(sizeof(float*) * plugin->numOutputs );
    else
        audioOut = nullptr;

    _host->set_event_handler(_host->get_metaplugin(), this);
}




bool VstAdapter::invoke(zzub_event_data_t& data) {
    if (!plugin || data.type != zzub::event_type_double_click || !(info->flags & zzub_plugin_flag_has_custom_gui))
        return false;

    if(is_editor_open)
        return true;

    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), this);

    gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(_host->get_host_info()->host_ptr));
    gtk_window_set_title(GTK_WINDOW(window), info->name.c_str());

    WIN_ID_TYPE win_id = WIN_ID_FUNC(window);

    dispatch(plugin, effEditOpen, (void*) win_id);
    is_editor_open = true;

    return true;
}

void VstAdapter::ui_destroy() {
    dispatch(plugin, effEditClose);
    is_editor_open = false;
}

bool VstAdapter::process_stereo(float **pin, float **pout, int numsamples, int mode) {
    if(mode != zzub::process_mode_no_io)
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
