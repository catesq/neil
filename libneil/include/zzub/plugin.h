// zzub Plugin Interface
// Copyright (C) 2006 Leonard Ritter (contact@leonard-ritter.com)
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

#include <cassert>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>

#include <cstdint>


#include "zzub/consts.h"

#include "zzub/attribute.h"
#include "zzub/info.h"
#include "zzub/internal.h"
#include "zzub/parameter.h"


#include "libzzub/archive.h"
#include "libzzub/host.h"
#include "libzzub/streams.h"
#include "libzzub/sequence_info.h"
#include "libzzub/wave_info.h"


namespace zzub {


struct pattern;
struct player;
struct song;
struct info;

struct event_handler {
    virtual ~event_handler() {}
    virtual bool invoke(zzub_event_data_t &data) = 0;
};

struct plugin {
    virtual ~plugin() {}
    virtual void destroy() { delete this; }
    virtual void init(zzub::archive *arc) {}
    virtual void created() {}
    virtual void process_events() {}
    virtual void process_midi_events(midi_message *pin, int nummessages) {}
    virtual void process_controller_events() {}
    virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode) { return false; }
    virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
    virtual void stop() {}
    virtual void load(zzub::archive *arc) {}
    virtual void save(zzub::archive *arc) {}
    virtual void attributes_changed() {}
    virtual void command(int index) {}
    virtual void set_track_count(int count) {}
    virtual void mute_track(int index) {}
    virtual bool is_track_muted(int index) const { return false; }
    virtual void midi_note(int channel, int value, int velocity) {}
    virtual void event(unsigned int data) {}
    virtual const char *describe_value(int param, int value) { return 0; }
    virtual const zzub::envelope_info **get_envelope_infos() { return 0; }
    virtual bool play_wave(int wave, int note, float volume, int offset, int length) { return false; }
    virtual void stop_wave() {}
    virtual int get_wave_envelope_play_position(int env) { return -1; }

    // these have been in zzub::plugin2 before
    virtual const char *describe_param(int param) { return 0; }
    virtual bool set_instrument(const char *name) { return false; }
    virtual void get_sub_menu(int index, zzub::outstream *os) {}
    virtual void add_input(const char *name, zzub::connection_type type) {}
    virtual void delete_input(const char *name, zzub::connection_type type) {}
    virtual void rename_input(const char *oldname, const char *newname) {}
    virtual void input(float **samples, int size, float amp) {}
    virtual void midi_control_change(int ctrl, int channel, int value) {}
    virtual bool handle_input(int index, int amp, int pan) { return false; }
    // plugin_flag_has_midi_output
    virtual void get_midi_output_names(outstream *pout) {}

    // plugin_flag_stream | plugin_flag_has_audio_output
    virtual void set_stream_source(const char *resource) {}
    virtual const char *get_stream_source() { return 0; }

    virtual void play_sequence_event(zzub_sequence_t *seq, const sequence_event &ev, int offset) {}

    // Called by the host to set specific configuration options,
    // usually related to paths.
    virtual void configure(const char *key, const char *value) {}

    virtual const char *get_preset_file_extensions() { return nullptr; }
    virtual bool load_preset_file(const char *) { return false; }
    virtual bool save_preset_file(const char *) { return false; }  // return pointer to data to be writter to a preset file - the memory will be freed after the preset file is saved

    plugin() {
        global_values = 0;
        track_values = 0;
        controller_values = 0;
        attributes = 0;
        _master_info = 0;
        _host = 0;
    }

    void *global_values;
    void *track_values;
    void *controller_values;
    int *attributes;

    master_info *_master_info;
    host *_host;
};

// A plugin factory allows to add and replace plugin infos
// known to the host.
struct pluginfactory {
    // Registers a plugin info to the host. If the uri argument
    // of the info struct designates a plugin already existing
    // to the host, the old info struct will be replaced.
    virtual void register_info(const zzub::info *_info) = 0;
};

// A plugin collection registers plugin infos and provides
// serialization services for plugin info, to allow
// loading of plugins from song data.
struct plugincollection {
    virtual ~plugincollection() {}

    // Called by the host initially. The collection registers
    // plugins through the pluginfactory::register_info method.
    // The factory pointer remains valid and can be stored
    // for later reference.
    virtual void initialize(zzub::pluginfactory *factory) {}

    // Called by the host upon song loading. If the collection
    // can not provide a plugin info based on the uri or
    // the metainfo passed, it should return a null pointer.
    // This will usually only be called if the host does
    // not know about the uri already.
    virtual const zzub::info *get_info(const char *uri, zzub::archive *arc) { return 0; }

    // Returns the uri of the collection to be identified,
    // return zero for no uri. Collections without uri can not be
    // configured.
    virtual const char *get_uri() { return 0; }

    // Called by the host to set specific configuration options,
    // usually related to paths.
    virtual void configure(const char *key, const char *value) {}

    // Called by the host upon destruction. You should
    // delete the instance in this function
    virtual void destroy() { delete this; }
};


}  




/*
  in case you are using this header to write a plugin,
  you need to compile your library with a .def file that
  looks like this:

  EXPORTS
  zzub_get_infos
  zzub_get_signature
*/





extern "C" const char *zzub_get_signature();

extern "C" zzub::plugincollection *zzub_get_plugincollection();

typedef zzub::plugincollection *(*zzub_get_plugincollection_function)();
typedef const char *(*zzub_get_signature_function)();


