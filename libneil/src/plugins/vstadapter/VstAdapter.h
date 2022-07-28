#pragma once

#include <array>
#include "zzub/plugin.h"
#include "VstPluginInfo.h"
#include <boost/dll.hpp>
#include <gtk/gtk.h>

#define MAX_TRACKS 16


struct tvals {
  uint8_t note;
  uint8_t volume;
} __attribute__((__packed__));

extern "C" {
    void on_window_destroy(GtkWidget* widget, gpointer data);
}


struct VstAdapter : zzub::plugin, zzub::event_handler {
    VstAdapter(const VstPluginInfo* info);
    ~VstAdapter();
    void init(zzub::archive* pi);
    bool process_stereo(float **pin, float **pout, int n, int mode);
    const char *describe_value(int param, int value);
    bool invoke(zzub_event_data_t& data);
    void ui_destroy();

private:
    float** audioIn;
    float** audioOut;
    unsigned num_tracks = 0;
    std::array<tvals, MAX_TRACKS> trackvals;
    uint16_t* globalvals;
    boost::dll::shared_library lib{};
    bool is_editor_open = false;
    AEffect* plugin;
    const VstPluginInfo* info;
};
