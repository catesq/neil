#pragma once

#include <gtk/gtk.h>

#include <array>
#include <boost/dll.hpp>

#include "vst_defines.h"
#include "zzub/plugin.h"


struct vst_zzub_info;


extern "C" {
    void on_window_destroy(GtkWidget* widget, gpointer data);
}


struct vst_adapter : zzub::plugin, zzub::event_handler, zzub::midi_plugin_interface {
    vst_adapter(const vst_zzub_info* info);
    virtual ~vst_adapter();
    virtual void init(zzub::archive* pi) override;
    virtual void created() override;
    virtual void save(zzub::archive*) override;
    virtual bool process_stereo(float** pin, float** pout, int n, int mode) override;
    virtual const char* describe_value(int param, int value) override;
    virtual bool invoke(zzub_event_data_t& data) override;
    virtual void process_events() override;
    virtual void set_track_count(int track_count) override;
    virtual const char* get_preset_file_extensions() override;
    virtual bool load_preset_file(const char*) override;
    virtual bool save_preset_file(const char*) override;

    void clear_vst_events();

    VstTimeInfo* get_vst_time_info();

    void parameter_update_from_ui(int index, float value);

    void ui_open();
    void ui_resize( int width, int height );
    void ui_destroy();

    uint64_t sample_pos = 0;

    // call plugin->getParameter - convert the floating point value returned from the vst plugin into a byte or word len integer used by the globalvalues of the zzub plugin adapter
    // if dispatch_control_change == true then call zzub::player::control_change()
    // control_change() segfaults when called from zzub::plugin::init()
    //    void update_zzub_globals_from_plugin(bool dispatch_control_change = true);

    // if a valid archive is given to the init function then get the read the vst parameter values from the save file
    void init_from_archive(zzub::archive*);

    virtual void add_note_on(uint8_t note, uint8_t volume) override;
    virtual void add_note_off(uint8_t note) override;
    virtual void add_aftertouch(uint8_t note, uint8_t volume) override;
    virtual void add_midi_command(uint8_t cmd, uint8_t data1, uint8_t data2) override;
    virtual zzub::midi_note_track* get_track_data_pointer(uint16_t track_num) const override;

    float get_ui_scale() { return ui_scale;}

private:
    bool initialized = false;

    zzub::midi_track_manager midi_track_manager;
    int active_index = -1;  // keep track of which parameter index is being adjusted (see audioMasterBeginEdit audioMasterEndEdit)
                            // the octasine plugin - or the vst-rs module - sends a burst of spurious EndEdit messages -
                            // with inaccurate indexes - *after* the EndEdit with the correct index has been sent
                            // active_index is used to filter these out. -1 indicates no parameter being changed
    float** audioIn;
    float** audioOut;
    unsigned num_tracks = 0;

    attrvals attr_values{0, 0};

    uint16_t* globalvals;
    boost::dll::shared_library lib{};
    bool is_editor_open = false;
    GtkWidget* window = nullptr;
    AEffect* plugin = nullptr;
    float ui_scale = 1.0f;

    const vst_zzub_info* info;
    std::vector<VstMidiEvent*> midi_events;
    VstEvents* vst_events;
    VstTimeInfo vst_time_info{};
    zzub_plugin_t* metaplugin = nullptr;
};
