#pragma once

#include "suil/suil.h"

#include "lv2_defines.h"
#include "lv2_utils.h"
#include "ext/lv2_evbuf.h"
#include "Ports.h"
#include "PluginInfo.h"

#include "zzub/zzub.h"
#include "zzub/plugin.h"
#include "zzub/signature.h"

#include <vector>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include <gdk/gdk.h>
#include <glib-object.h>
#include <glib.h>
#include <gobject/gclosure.h>
#include <gtk/gtk.h>

#ifndef _WIN32
    #define HAVE_MLOCK 1      // zix checks for HAVE_MLOCK before using mlock - in sys/mman.h - for memory locking
#endif

#include "zix/common.h"
#include "zix/ring.h"
#include "zix/sem.h"

#include "ext/lv2_programs.h"

#include "features/worker.h"


inline float get_scale_factor() {
    GtkWidget* test_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    float scale_factor =  gtk_widget_get_scale_factor(test_window);
    gtk_widget_destroy(test_window);
    return scale_factor;
}


void program_changed(
        LV2_Programs_Handle handle,
        int32_t index
);

const void* get_port_value(
        const char* port_symbol,
        void*       user_data,
        uint32_t*   size,
        uint32_t*   type
);


void set_port_value(
        const char* port_symbol,
        void*       user_data,
        const void* value,
        uint32_t    size,
        uint32_t    type
);


void write_events_from_ui(
        void* const adapter_handle,
        uint32_t    port_index,
        uint32_t    buffer_size,
        uint32_t    protocol,
        const void* buffer
);


uint32_t lv2_port_index(
        void* const lv2adapter_handle,
        const char* symbol
);


struct PluginInfo;
struct ParamPort;
struct SharedCache;

extern "C" {
    void on_window_destroy(GtkWidget* widget, gpointer data);
    void finalise_plugin_ui(GtkWidget* widget, gpointer data);
}

struct PluginAdapter : zzub::plugin, zzub::event_handler {
    // zzub engine boilerplate - trak_states are the previous plugin port values, trak_values are the new port values. attr_values are legacy.
    trackvals       trak_values[16]{};
    trackvals       trak_states[16]{};
    attrvals        attr_values;

    PluginInfo*     info              = nullptr;
    SharedCache*    cache             = nullptr;
    LilvInstance*   lilvInstance      = nullptr;
    zzub_plugin_t*  metaPlugin        = nullptr;

    LilvUIs*        uis               = nullptr;
    const LilvUI*   lilv_ui_type      = nullptr;
    const LilvNode* lilv_ui_type_node = nullptr;
    SuilHost*       suil_ui_host      = nullptr;    // < Plugin UI host support
    SuilInstance*   suil_ui_instance  = nullptr;    // < Plugin UI instance (shared library)
    SuilHandle      suil_ui_handle    = nullptr;
    GtkWidget*      gtk_ui_window     = nullptr;
    GtkWidget*      gtk_ui_root_box   = nullptr;
    GtkWidget*      gtk_ui_parent_box = nullptr;
    void*           transient_wid     = nullptr;

    uint32_t        samp_count        = 0;         //number of samples played
    uint32_t        last_update       = 0;
    uint32_t        update_every      = 767;       //update from ui after every x samples
    int32_t         trackCount        = 0;

    float           ui_scale          = 2.0;       // for displaying ui of plugins on high density displays. only updated when the ui_window is created in PluginAdapter::invoke
    float           sample_rate       = 44100;
    float           update_rate       = 10;

    MidiEvents      midiEvents{};
    bool            program_change_update = false;
    bool            halting                    = false;

    LV2Features     features;
    LV2Worker       worker;
    ZixRing*        ui_events;       // Port events from ui
    ZixRing*        plugin_events;   // Port events from plugin
    ZixSem          work_lock;       // lock for the LV2Worker

    std::vector<Port*>         ports;

    std::vector<AudioBufPort*> audioInPorts;
    std::vector<AudioBufPort*> audioOutPorts;

    std::vector<AudioBufPort*> cvPorts;

    std::vector<EventBufPort*> eventPorts;
    std::vector<EventBufPort*> midiPorts;

    std::vector<ControlPort*>  controlPorts;
    std::vector<ParamPort*>    paramPorts;


    PluginAdapter(PluginInfo *info);
    ~PluginAdapter();


    void                connect(LilvInstance* pluginInstance);
    void                ui_destroy();
    void                update_all_from_ui();
    
    virtual bool        invoke(zzub_event_data_t& data);
    virtual void        destroy();
    virtual void        init(zzub::archive *arc);
    virtual void        process_events();
    virtual const char* describe_value(int param, int value);
    virtual void        set_track_count(int ntracks);
    virtual void        stop();
    virtual bool        process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate);
    virtual bool        process_stereo(float **pin, float **pout, int numsamples, int const mode);
    virtual void        load(zzub::archive *arc);
    virtual void        save(zzub::archive *arc);

    ParamPort*          get_param_port(std::string symbol);

private:
    bool       ui_open();
    bool       is_ui_resizable();
    bool       isExternalUI(const LilvUI* ui);
    void       process_all_midi_tracks();
    void       process_one_midi_track(midi_msg &vals_msg, midi_msg& state_msg);
    void       update_port(ParamPort* port, float float_val);
    void       ui_event_import();
    const bool ui_select(const char *native_ui_type, const LilvUI** ui_type_ui, const LilvNode** ui_type_node);
    GtkWidget* ui_open_window(GtkWidget** root_container, GtkWidget** parent_container);
    void       init_static_features();

    const LV2UI_Idle_Interface* idle_interface = nullptr;
    const LV2UI_Show_Interface* show_interface = nullptr;
    bool showing_interface                     = false;

    // virtual void process_controller_events();
    // virtual void attributes_changed();
    // virtual void command(int);
    // virtual void mute_track(int);
    // virtual bool is_track_muted(int) const;
    // virtual void event(unsigned int);
    // virtual const zzub::envelope_info** get_envelope_infos();
    // virtual bool play_wave(int, int, float);
    // virtual void stop_wave();
    // virtual int get_wave_envelope_play_position(int);
    // virtual const char* describe_param(int);
    // virtual bool set_instrument(const char*);
    // virtual void get_sub_menu(int, zzub::outstream*);
    // virtual void add_input(const char*);
    // virtual void delete_input(const char*);
    // virtual void rename_input(const char*, const char*);
    // virtual void input(float**, int, float);
    // virtual void midi_control_change(int, int, int);
    // virtual bool handle_input(int, int, int);


};

