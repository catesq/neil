#pragma once

#include <array>
#include "zzub/plugin.h"
#include "VstPluginInfo.h"
#include <boost/dll.hpp>
#include <gtk/gtk.h>
#include "VstDefines.h"

#define MAX_TRACKS 16
#define MAX_EVENTS 256






extern "C" {
    void on_window_destroy(GtkWidget* widget, gpointer data);
}


struct VstAdapter : zzub::plugin, zzub::event_handler {
    VstAdapter(const VstPluginInfo* info);
    virtual ~VstAdapter();
    virtual void init(zzub::archive* pi) override;
    virtual bool process_stereo(float **pin, float **pout, int n, int mode) override;
    virtual const char *describe_value(int param, int value) override;
    virtual bool invoke(zzub_event_data_t& data) override;
    virtual void process_events() override;
    virtual void set_track_count(int track_count) override;
    virtual const char* get_preset_file_extensions() override;
    virtual bool load_preset_file(const char*) override;
    virtual bool save_preset_file(const char*) override;  // return pointer to data to be writter to a preset file - the memory will be freed after the preset file is saved

    void clear_vst_events();

    VstTimeInfo* get_vst_time_info();

    void update_parameter_from_gui(int index, float value);

    void ui_destroy();

    uint64_t sample_pos = 0;

    void update_zzub_globals_from_plugin();

private:
    void process_one_midi_track(midi_msg &vals_msg, midi_msg& state_msg);
    void process_midi_tracks();

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
    AEffect* plugin = nullptr;
    float ui_scale = 1.0f;
    bool update_zzub_params = false;

    const VstPluginInfo* info;
    std::vector<VstMidiEvent*> midi_events;
    VstEvents* vst_events;
    VstTimeInfo vst_time_info{};
};
