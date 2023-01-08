#pragma once

#include "suil/suil.h"

#include "lv2_defines.h"
#include "lv2_utils.h"
#include "lv2_ports.h"
#include "lv2_zzub_info.h"

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

#include "zix/ring.h"
#include "zix/sem.h"

#include "features/worker.h"


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


struct lv2_zzub_info;
struct ParamPort;
struct SharedCache;


extern "C" 
{
//    bool on_window_destroy(GtkWidget* widget, gpointer data);

    // event handle for close window button.
    void       ui_close(GtkWidget* widget, GdkEventButton* event, gpointer data);

}


struct lv2_adapter : zzub::plugin, zzub::event_handler 
{
    std::vector<lv2_port*>         ports;

    std::vector<audio_buf_port*>   audioInPorts;
    std::vector<audio_buf_port*>   audioOutPorts;
    std::vector<audio_buf_port*>   cvPorts;

    std::vector<event_buf_port*>   eventPorts;
    std::vector<event_buf_port*>   midiPorts;

    std::vector<control_port*>     controlPorts;
    std::vector<param_port*>       paramPorts;

    // zzub engine boilerplate - trak_states are the previous plugin port values, trak_values are the new port values. attr_values are legacy.
    trackvals         trak_values[16]{};
    trackvals         trak_states[16]{};
    attrvals          attr_values{0,0};
    bool              initialized           = false;
    bool              ui_is_open            = false;
    bool              program_change_update = false;
    bool              halting               = false;
    lv2_zzub_info*    info              = nullptr;
    lv2_lilv_world*   cache             = nullptr;
    LilvInstance*     lilvInstance      = nullptr;
    zzub_plugin_t*    metaPlugin        = nullptr;
    LilvUIs*          uis               = nullptr;
    const LilvUI*     lilv_ui_type      = nullptr;
    const LilvNode*   lilv_ui_type_node = nullptr;
    SuilHost*         suil_ui_host      = nullptr;    // < Plugin UI host support
    SuilInstance*     suil_ui_instance  = nullptr;    // < Plugin UI instance (shared library)
    SuilHandle        suil_ui_handle    = nullptr;
    GtkWidget*        gtk_ui_window     = nullptr;
    GtkWidget*        gtk_ui_root_box   = nullptr;
    GtkWidget*        gtk_ui_parent_box = nullptr;
    GtkWidget*        suil_widget       = nullptr;
    void*             transient_wid     = nullptr;
    uint32_t          samp_count        = 0;         //number of samples played
    uint32_t          last_update       = 0;
    uint32_t          update_every      = 767;       //update from ui after every x samples
    int32_t           trackCount        = 0;
    float             ui_scale          = 2.0;       // for displaying ui of plugins on high density displays. only updated when the ui_window is created in PluginAdapter::invoke
    float             sample_rate       = 44100;
    float             update_rate       = 10;
    MidiEvents        midiEvents{};

    LV2Features       features;
    LV2Worker         worker;
    ZixRing*          ui_events;       // Port events from ui
    ZixRing*          plugin_events;   // Port events from plugin
    ZixSem            work_lock;       // lock for the LV2Worker

    lv2_adapter(lv2_zzub_info *info);
    ~lv2_adapter();

    void                connect(LilvInstance* pluginInstance);
    void                update_all_from_ui();
    
    virtual bool        invoke(zzub_event_data_t& data);
    virtual void        destroy();
    virtual void        init(zzub::archive *arc);
    virtual void        created();
    virtual void        process_events();
    virtual const char* describe_value(int param, int value);
    virtual void        set_track_count(int ntracks);
    virtual void        stop();
    virtual bool        process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate);
    virtual bool        process_stereo(float **pin, float **pout, int numsamples, int const mode);
    virtual void        save(zzub::archive *arc);

    param_port*         get_param_port(std::string symbol);

private:

    void       ui_open();
    void       ui_reopen();
    void       ui_destroy();
    bool       is_ui_resizable();
    bool       is_ui_external(const LilvUI* ui);
    void       process_all_midi_tracks();
    void       process_one_midi_track(midi_msg &vals_msg, midi_msg& state_msg);
    void       update_port(param_port* port, float float_val);

    void       read_archive_params(zzub::instream* instream);
    void       read_archive_state(zzub::instream* instream, uint32_t length);
    void       save_archive_params(zzub::outstream *oustream);
    void       save_archive_state(zzub::outstream *oustream);

    bool       prefer_state_save() { return false; }

    //
    void       ui_event_import();

    // sends events to the ui - eg when a new patch has been loaded and all the controls have been changed
    void       ui_event_dispatch();

    const bool ui_select(const char *native_ui_type, const LilvUI** ui_type_ui, const LilvNode** ui_type_node);
    GtkWidget* ui_open_window(GtkWidget** root_container, GtkWidget** parent_container);
    void       init_static_features();

    //    const LV2UI_Idle_Interface* idle_interface = nullptr;
    //    const LV2UI_Show_Interface* show_interface = nullptr;
    //    bool showing_interface                     = false;
};

