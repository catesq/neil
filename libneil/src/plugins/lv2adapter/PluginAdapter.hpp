// -*- c-basic-offset: 8 -*-o
// dssi plugin adapter
// Copyright (C) 2006 Leonard Ritter (contact@leonard-ritter.com)
// Copyright (C) 2008 James McDermott (jamesmichaelmcdermott@gmail.com)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


#pragma once

#include "suil/suil.h"

#include "lv2_defines.h"
#include "lv2_utils.hpp"
#include "ext/lv2_evbuf.h"
#include "Ports.hpp"
#include "PluginInfo.hpp"

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

#include "zix/common.h"
#include "zix/ring.h"
#include "zix/sem.h"

#include "ext/lv2_programs.h"

using namespace zzub;

#define SUIL_HOST_UI "http://lv2plug.in/ns/extensions/ui#Gtk3UI"

void program_changed(LV2_Programs_Handle handle, int32_t index);

const void* get_port_value(const char* port_symbol,
                           void*       user_data,
                           uint32_t*   size,
                           uint32_t*   type);


void set_port_value(const char* port_symbol,
                    void*       user_data,
                    const void* value,
                    uint32_t    size,
                    uint32_t    type);


void write_events_from_ui(void* const adapter_handle,
                  uint32_t    port_index,
                  uint32_t    buffer_size,
                  uint32_t    protocol,
                  const void* buffer);


uint32_t lv2_port_index(void* const lv2adapter_handle, const char* symbol);


struct PluginInfo;
struct ParamPort;
struct PluginWorld;

extern "C" {
    void on_window_destroy(GtkWidget* widget, gpointer data);

}

struct PluginAdapter : plugin, event_handler {
    PluginInfo *info;
    PluginWorld *world;
	LilvInstance *pluginInstance = nullptr;
    zzub_plugin_t* metaPlugin = nullptr;

    const LilvUI* ui{};
    const LilvNode* ui_type{};
    SuilHost* ui_host{};            // < Plugin UI host support
	SuilInstance* ui_instance{};    // < Plugin UI instance (shared library)
    GtkWidget* ui_window{};
    GtkWidget* ui_container{};
    LV2_Programs_Host program_host{};  // pointers this  asnd program_select()
                                     // called when synth program changed in ui 
    
    uint32_t samp_count = 0;        //number of samples played
    uint32_t last_update = 0;
    uint32_t update_every = 767;    //update from ui after every x samples
	int trackCount = 0;
    tvals track_vals[16]{};
    tvals track_states[16]{};
    attrvals attr_vals;
    Lv2Features features{};
    bool update_from_program_change = false;

    ZixRing*         ui_events;       // Port events from ui
    ZixRing*         plugin_events;   // Port events from plugin

//	unsigned evtBufSize = 0;
    // std::vector<Lv2Port*> ports;

    // std::vector<float>dataValues;
    float *values;

    float *audioBufs = nullptr;
    float *audioIn, *audioOut;
    float *cvBufs = nullptr;

    std::vector<LV2_Evbuf*> midiBufs{};
    // std::vector<LV2_Atom_Sequence*> midiSeqs;
    std::vector<LV2_Evbuf*> eventBufs{};

    MidiEvents midiEvents{};

    ~PluginAdapter();

    PluginAdapter(PluginInfo *info);

    ParamPort* get_param_port(std::string symbol);

    void connectInstance(LilvInstance* pluginInstance);
    void connectPluginUI();
    
    virtual bool invoke(zzub_event_data_t& data);
    virtual void destroy();
    virtual void init(zzub::archive *arc);
    void read_from_archive(zzub::archive *arc);
    virtual void process_events();

    virtual const char * describe_value(int param, int value) override;
    virtual void set_track_count(int ntracks);
    virtual void stop();
    virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate);
    virtual bool process_stereo(float **pin, float **pout, int numsamples, int const mode);

    virtual void load(zzub::archive *arc);
    virtual void save(zzub::archive *arc);

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
    
    void destroy_ui();
    void update_all_from_ui();
private:
    void process_track_midi_events(midi_msg &vals_msg, midi_msg& state_msg);
    void update_port(ParamPort* port, float float_val);
    void apply_events_from_ui();
    const LilvUI* select_ui();
    GtkWidget* open_ui(GtkWidget* window);
    void attach_ui(GtkWidget* window, GtkWidget* container);
    void init_static_features();
};

