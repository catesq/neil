#pragma once

#include <array>
#include "zzub/plugin.h"
#include "VstPluginInfo.h"
#include <boost/dll.hpp>
#include <gtk/gtk.h>
#include "VstDefines.h"

#define MAX_TRACKS 16
#define MAX_EVENTS 256


struct tvals {
  uint8_t note = zzub::note_value_none;
  uint8_t volume = VOLUME_NONE;
} __attribute__((__packed__));

extern "C" {
    void on_window_destroy(GtkWidget* widget, gpointer data);
}


struct VstAdapter : zzub::plugin, zzub::event_handler {
    VstAdapter(const VstPluginInfo* info);
    virtual ~VstAdapter();
    virtual void init(zzub::archive* pi);
    virtual bool process_stereo(float **pin, float **pout, int n, int mode);
    virtual const char *describe_value(int param, int value);
    virtual bool invoke(zzub_event_data_t& data);
    virtual void process_events();
    void clear_vst_events();

    VstTimeInfo* get_vst_time_info(bool update = false);

    void update_parameter_from_gui(int index, float value);

    void ui_destroy();

    uint64_t sample_pos = 0;

private:
    int active_index = -1;   // keep track of which parameter index is being adjusted (see audioMasterBeginEdit audioMasterEndEdit). -1 indicates no parameter being changed
                             // the octasine plugin - or the vst-rs module - sends a burst spurious EndEdit messages, with inaccurate indexes, *after* the EndEdit with the correct index has been sent
                             // use the active_index to filter these out...
    float** audioIn;
    float** audioOut;
    unsigned num_tracks = 0;
    std::array<tvals, MAX_TRACKS> trackvalues;
    std::array<tvals, MAX_TRACKS> trackstates;

    uint16_t* globalvals;
    boost::dll::shared_library lib{};
    bool is_editor_open = false;
    GtkWidget* window = nullptr;
    AEffect* plugin;
    const VstPluginInfo* info;
    std::vector<VstMidiEvent*> midi_events;
    VstEvents* vst_events;
    VstTimeInfo vst_time_info{};
};
