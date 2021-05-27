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


// lv2adapter allows running DSSI plugins as zzub plugins

#include "lv2_defines.h"
#include "lv2_utils.hpp"
#include "PluginAdapter.hpp"
#include "PluginInfo.hpp"
#include <ostream>
#include <string>


#include <gdk/gdk.h>
#include <glib-object.h>
#include <glib.h>
#include <gobject/gclosure.h>
#include <gtk/gtk.h>
#include "zzub/plugin.h"
#include "lv2/state/state.h"
#include "lv2/atom/atom.h"

const char *zzub_adapter_name = "zzub lv2 adapter";

PluginAdapter::PluginAdapter(PluginInfo *info) : info(info), world(info->world) {
    if (verbose) { printf("plugin <%s>: in constructor\n", info->name.c_str()); }

    if(info->zzubGlobalsLen) {
        global_values = malloc(info->zzubGlobalsLen);
        memset(global_values, 0, info->zzubGlobalsLen);
    }

    if (info->paramPorts.size() > 0) {
        values = (float*) malloc(sizeof(float) * info->paramPorts.size());
    }

	ui_events     = zix_ring_new(EVENT_BUF_SIZE);
	plugin_events = zix_ring_new(EVENT_BUF_SIZE);
    
    track_values = track_vals;
    attributes = (int *) &attr_vals;

    if (info->audioPorts.size() > 0) {
        audioBufs = (float *) malloc(info->audioPorts.size() * sizeof(float*) * ZZUB_BUFLEN);
        memset(audioBufs, 0.0f, sizeof(float) * ZZUB_BUFLEN * info->audioPorts.size());
        audioIn = audioBufs;
        audioOut = audioBufs + info->audio_in_count * ZZUB_BUFLEN;
    }

    if(info->cvPorts.size() > 0) {
        cvBufs = (float *) malloc(info->cvPorts.size() * ZZUB_BUFLEN * sizeof(float));
        memset(cvBufs, 0.0f, sizeof(float) * ZZUB_BUFLEN * info->cvPorts.size());
    }

    // for(EventPort *evtPort: info->eventPorts) {
    //     eventBufs.push_back(lv2_evbuf_new(world->hostParams.bufSize, world->urids.atom_Chunk, world->urids.atom_Sequence));
    // }

    for(EventPort *midiPort: info->midiPorts) {
        midiBufs.push_back(lv2_evbuf_new(world->hostParams.bufSize, world->urids.atom_Chunk, world->urids.atom_Sequence));
        // midiSeqs.push_back((LV2_Atom_Sequence*)malloc(sizeof(LV2_Atom_Sequence) + world->hostParams.bufSize));
    }

    if(info->flags | zzub::plugin_flag_has_custom_gui) {
        world->init_suil();
    }

    init_static_features();

    if(verbose) { printf("Construted buffers. audio & cv buflen: %u, event buflen: %i.\n", ZZUB_BUFLEN, world->hostParams.bufSize); }
}

PluginAdapter::~PluginAdapter() {
    if (verbose)
        printf("in PluginAdapter::~PluginAdapter() %lu host %lu metaplugin\n", metaPlugin, _host);

    if (pluginInstance)
        lilv_instance_free(pluginInstance);

    if (global_values)
        free(global_values);

    if (audioBufs != nullptr)
        free(audioBufs);

    if (cvBufs != nullptr)
        free(cvBufs);

    for(auto buf: midiBufs)
        lv2_evbuf_free(buf);

    // for(auto seq: midiSeqs)
    //     lilv_free(seq);

    for(auto buf: eventBufs)
        lv2_evbuf_free(buf);

    midiBufs.clear();
    // midiSeqs.clear();
    eventBufs.clear();
}


void PluginAdapter::init_static_features() {
    Lv2HostParams& host_params = this->world->hostParams;
    SymapUrids& urids = this->world->urids;

    LV2_Options_Option options[NUM_OPTIONS] = {
       { LV2_OPTIONS_INSTANCE, 0, urids.param_sampleRate,   sizeof(float), urids.atom_Float, &host_params.sample_rate },
       { LV2_OPTIONS_INSTANCE, 0, urids.minBlockLength,     sizeof(int32_t), urids.atom_Int, &host_params.minBlockLength },
       { LV2_OPTIONS_INSTANCE, 0, urids.maxBlockLength,     sizeof(int32_t), urids.atom_Int, &host_params.blockLength },
       { LV2_OPTIONS_INSTANCE, 0, urids.bufSeqSize,         sizeof(int32_t), urids.atom_Int, &host_params.bufSize },
       { LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, NULL }
    };

    memcpy(features.options, options, sizeof(features.options));

    features.uri_map_feature.data      = &this->world->uri_map;
    features.map_feature.data          = &this->world->map;
    features.unmap_feature.data        = &this->world->unmap;
    features.bounded_buf_feature.data  = &host_params.blockLength;
    features.options_feature.data      = features.options;
    features.make_path_feature.data    = &this->world->make_path;
    features.program_host_feature.data = &this->program_host;

    program_host.handle = this;
    program_host.program_changed = &program_changed;
}



void PluginAdapter::init(zzub::archive *arc) {
    // if (verbose) 
    const LV2_Feature* feature_list[7] = {
        &features.uri_map_feature,
		&features.map_feature,
        &features.program_host_feature,
		&features.unmap_feature,
		// &features.sched_feature,
		&features.options_feature,
        // &features.pow2_buf_feature,
        // &features.fixed_buf_feature,
        &features.bounded_buf_feature,
        // &features.default_state_feature,
		NULL
	};

    pluginInstance = lilv_plugin_instantiate(info->lilvPlugin, _master_info->samples_per_second, feature_list);

    metaPlugin = _host->get_metaplugin();
    _host->set_event_handler(metaPlugin, this);
    printf("connect plugin %s\n", info->name.c_str());
    connectInstance(pluginInstance);
    
    lilv_instance_activate(pluginInstance);

    features.ext_data.data_access = lilv_instance_get_descriptor(pluginInstance)->extension_data;

    // read from the archive
    if (arc) {
        // read_from_archive(arc);
    }
}


const LilvUI* PluginAdapter::select_ui() {
    const char *native_ui_uri = GTK3_URI;
    // Try to find an embeddable UI
    LilvNode* native_type = lilv_new_uri(world->lilvWorld, native_ui_uri);

    LILV_FOREACH (uis, u, info->uis) {
        const LilvUI*   ui        = lilv_uis_get(info->uis, u);
        const LilvNode* type      = NULL;
        const bool      supported = lilv_ui_is_supported(ui, suil_ui_supported, native_type, &type);
        
        if (supported) {
            lilv_node_free(native_type);
            this->ui_type = type;
            return ui;
        }
    }

    lilv_node_free(native_type);
    return nullptr;
}

void on_window_destroy(GtkWidget* widget, gpointer data) {
    PluginAdapter* self = (PluginAdapter*)data;
    self->destroy_ui();
}

void PluginAdapter::destroy_ui() {
    gtk_widget_destroy(GTK_WIDGET(ui_window));
    suil_instance_free(ui_instance);
    suil_host_free(ui_host);
    ui_window    = nullptr;
    ui_container = nullptr;
    ui_host      = nullptr;
    ui_instance = nullptr;
}


void PluginAdapter::attach_ui(GtkWidget* window, GtkWidget* container) {
    GtkWidget* widget = (GtkWidget*)suil_instance_get_widget(ui_instance);

    gtk_container_add(GTK_CONTAINER(container), widget);
    gtk_window_set_resizable(GTK_WINDOW(window), true);
    gtk_widget_show_all(container);
    gtk_widget_grab_focus(widget);
    gtk_widget_show_all(window);
}


GtkWidget* PluginAdapter::open_ui(GtkWidget* window) {
	g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), this);

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	gtk_window_set_role(GTK_WINDOW(window), "plugin_ui");
	gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget* alignment = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);
	gtk_box_pack_start(GTK_BOX(vbox), alignment, TRUE, TRUE, 0);
	
    // printf("return from open_ui\n");
    return alignment;
}


void program_changed(LV2_Programs_Handle handle, int32_t index) {
    // printf("program changed\n");
    PluginAdapter* adapter = (PluginAdapter*) handle;
    adapter->update_all_from_ui();
    adapter->update_from_program_change = true;
}


bool PluginAdapter::invoke(zzub_event_data_t& data) {
    if (data.type != zzub::event_type_double_click || !(info->flags & zzub_plugin_flag_has_custom_gui))
        return false;

    // printf("invoke ui_window %.X\n", ui_window);
    if(ui_window != nullptr) {
        return true;
    }

    // printf("show gui %d\n", data.type);
    ui_host   = suil_host_new(write_events_from_ui, lv2_port_index, nullptr, nullptr); //&update_port, &remove_port);
    ui        = select_ui();

    // some guis don't work. ie all gtk2 plugin in this gtk3 host
    if(!ui)
        return false;

    ui_window    = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    ui_container = open_ui(ui_window);

    const char* bundle_uri  = lilv_node_as_uri(lilv_ui_get_bundle_uri(ui));
	const char* binary_uri  = lilv_node_as_uri(lilv_ui_get_binary_uri(ui));
	char*       bundle_path = lilv_file_uri_parse(bundle_uri, NULL);
	char*       binary_path = lilv_file_uri_parse(binary_uri, NULL);

    const LV2_Feature instance_feature = {
		NS_EXT "instance-access", lilv_instance_get_handle(pluginInstance)
	};

    LV2_Feature parent_feature = {
		LV2_UI__parent, ui_window
	};

    const LV2_Feature* ui_features[8] = {
        &features.uri_map_feature,
		&features.map_feature,
		&features.unmap_feature,
        &features.program_host_feature,
		&instance_feature,
		// &data_feature,
		// &features.log_feature,
		&parent_feature,
		&features.options_feature,
		// &idle_feature,
		// &features.request_value_feature,
		NULL
	};

    ui_instance = suil_instance_new(
		ui_host,
		this,
		GTK3_URI,
		lilv_node_as_uri(lilv_plugin_get_uri(info->lilvPlugin)),
		lilv_node_as_uri(lilv_ui_get_uri(ui)),
		lilv_node_as_uri(ui_type),
		bundle_path,
		binary_path,
		ui_features);

	lilv_free(binary_path);
	lilv_free(bundle_path);

    if(!ui_instance) 
        return false;

    attach_ui(ui_window, ui_container);
    gtk_window_present(GTK_WINDOW(ui_window));
    gtk_main();

    	// Set initial control port values
	for (auto port: info->paramPorts) {
        suil_instance_port_event(
            ui_instance, 
            port->index,
            sizeof(float), 
            0, 
            &values[port->paramIndex]
        );
	}

    return true;
}

void PluginAdapter::destroy() {
//    if (verbose) printf("in destroy\n");
//    if(pluginInstance) { lilv_instance_free(pluginInstance); }
    delete this;
}


//float PluginAdapter::get_param_value(std::string symbol) {
//    ParamPort *port = get_param_port(symbol);

//    return port == nullptr ? 0.f : port->zzub_to_lilv_value(port->getData((uint8_t*) global_values));

//    //    uint8_t* globals = (uint8_t*) global_values;

//    //    switch(port->zzubParam->type) {
//    //    case zzub_parameter_type_word:
//    //        return port->toLv2Val(*(uint16_t*)(globals + port->byteOffset));
//    //    case zzub_parameter_type_byte:
//    //        return port->toLv2Val(*(globals + port->byteOffset));
//    //    case zzub_parameter_type_note:
//    //        return (float) *((uint8_t*) (globals + port->byteOffset));
//    //    case zzub_parameter_type_switch:
//    //        return (float) *((uint8_t*) (globals + port->byteOffset)) == zzub::switch_value_off ? 0.f : 1.f;
//    //    }
//}

//void PluginAdapter::set_param_value(std::string symbol, float value) {
//    printf("\nSet param value\n");
//    ParamPort *port = get_param_port(symbol);

//    if(port != nullptr) {
//        port->putData((uint8_t*) global_values, port->lilv_to_zzub_value(value));
//    }


//    //    switch(port->zzubParam->type) {
//    //    case zzub_parameter_type_word:
//    //        *((uint16_t*) (globals + port->byteOffset)) = port->toZzubVal(value);
//    //    case zzub_parameter_type_byte:
//    //        *(globals + port->byteOffset) = port->toZzubVal(value);
//    //    case zzub_parameter_type_note:
//    //        *(globals + port->byteOffset) = (u_int8_t) value;
//    //    case zzub_parameter_type_switch:
//    //        *((uint8_t*) (globals + port->byteOffset)) = value == 0.f ? zzub::switch_value_off : zzub::switch_value_on;
//    //    }
//}



void PluginAdapter::connectInstance(LilvInstance* pluginInstance) {
    if(verbose) { printf("in connectInstance\n"); }
    printf("connect instance %s\n", info->name.c_str());
    
    uint8_t* globals = (uint8_t*) global_values;
    for(auto paramPort: info->paramPorts) {
        values[paramPort->paramIndex] = paramPort->defaultVal;
        int zzubDefault = paramPort->zzubParam->value_default;

        switch(paramPort->zzubParam->type) {
        case zzub_parameter_type_note:
        case zzub_parameter_type_byte:
        case zzub_parameter_type_switch:
            *globals = (uint8_t) zzubDefault;
            break;
        case zzub_parameter_type_word:
            *((uint16_t*)globals) = (uint16_t) zzubDefault;
            break;
        }
        
        globals += paramPort->byteSize;

        lilv_instance_connect_port(pluginInstance, paramPort->index, &values[paramPort->paramIndex]);
    }

    for(auto audioPort: info->audioPorts) {
        lilv_instance_connect_port(pluginInstance, audioPort->index, &audioBufs[audioPort->bufIndex * ZZUB_BUFLEN]);
    }

    for(auto cvPort: info->cvPorts) {
        lilv_instance_connect_port(pluginInstance, cvPort->index, &cvBufs[cvPort->bufIndex * ZZUB_BUFLEN]);
    }

    for(auto midiPort: info->midiPorts) {
        lilv_instance_connect_port(pluginInstance, midiPort->index, lv2_evbuf_get_buffer(midiBufs[midiPort->bufIndex]));
    }
}



void PluginAdapter::read_from_archive(zzub::archive *arc) {
    // if(verbose)
    //  printf("in read_fromarchive");
    // zzub::instream *in = arc->get_instream("");
    // uint32_t len = 0;
    // char* str = (char*) malloc(len);
    // in->read(&len, 4);
    // in->read(str, len);

    // LilvState *state = lilv_state_new_from_string(world->lilvWorld, &world->map, str);
    // lilv_state_restore(state, pluginInstance, set_port_value, this, LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE, nullptr);
    // free(str);
    printf("read_from_archive\n");
}



void PluginAdapter::load(zzub::archive *arc) {
    // This is called when user selects a new preset in Aldrin.
    if (verbose) printf("PluginAdapter: in load preset()!\n");
    printf("load saved song settings for %s\n", info->name.c_str());
    printf("load ing\n");
}


void PluginAdapter::save(zzub::archive *arc) {
    // in osc_configure_handler we see what keys there are,
    // and for each key save the *latest* value.
    // then in update_handler we actually send the keys that we got in init()
    if (verbose) printf("PluginAdapter: in save()!\n");
    const char *dir = world->hostParams.tempDir;
    LilvState* const state = lilv_state_new_from_instance(
                info->lilvPlugin, pluginInstance, &world->map,
                dir, dir, dir, dir,
                get_port_value, this, LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE, nullptr);

    char *state_str = lilv_state_to_string(world->lilvWorld, &world->map, &world->unmap, state, "http://uri", nullptr);
    zzub::outstream *po = arc->get_outstream("");
    po->write((uint32_t) strlen(state_str));
    po->write(state_str, strlen(state_str));
}


const char *PluginAdapter::describe_value(int param, int value) {
//    if(verbose) {printf("in describe param %d %d\n", param, value);}
    static char text[256];
    if(param < info->paramPorts.size()) {
        return info->paramPorts[param]->describeValue(value, text);
    }
    return 0;
}

void PluginAdapter::set_track_count(int ntracks) {
//    if(verbose) {printf("in settrack count\n");}
    if (ntracks < trackCount) {
        for (int t = ntracks; t < trackCount; t++) {
            tvals &ts = track_states[t];
            if (ts.note != zzub::note_value_none) {
                midiEvents.noteOff(t, ts.note, 0);
            }
        }
    }
    trackCount = ntracks;
    if(verbose) { printf("done settrack\n"); }
}

ParamPort* PluginAdapter::get_param_port(std::string symbol) {
    Port* port = info->port_by_symbol(symbol);

    if(port->flow == PortFlow::Input && port->type == PortType::Control)
        return static_cast<ParamPort*>(port);
    else
        return nullptr;
}


void PluginAdapter::stop() {
}


void PluginAdapter::apply_events_from_ui() {
    ControlChange ev;

	const size_t  space = zix_ring_read_space(ui_events);
    for (size_t i = 0; i < space; i += sizeof(ev) + ev.size) {
        printf("apply ?\n");
		zix_ring_read(ui_events, (char*)&ev, sizeof(ev));
		char body[ev.size];  

		if (zix_ring_read(ui_events, body, ev.size) != ev.size) {
			fprintf(stderr, "error: Error reading from UI ring buffer\n");
			break;
		}
		assert(ev.index < jalv->num_ports);
		Port* port = info->ports[ev.index];

		if (ev.protocol == 0 && port->type == PortType::Control) {
            update_port((ParamPort*) port, *((float*) body));
		} else if (ev.protocol == world->urids.atom_eventTransfer) {
            printf("event from ui\n");
            if(port->type != PortType::Event)
                continue;

            EventPort* evt_port = (EventPort*) port;
			LV2_Evbuf_Iterator e = lv2_evbuf_end(eventBufs[evt_port->bufIndex]);

			const LV2_Atom* const atom = (const LV2_Atom*)body;
			lv2_evbuf_write(&e, samp_count, 0, atom->type, atom->size,
			                (const uint8_t*)LV2_ATOM_BODY_CONST(atom));
		} else {
			fprintf(stderr, "error: Unknown control change protocol %u\n",
			        ev.protocol);
		}
	}
}

void PluginAdapter::update_all_from_ui() {
    // const char *dir = world->hostParams.tempDir;

    //  LilvState* const state = lilv_state_new_from_instance(
    //             info->lilvPlugin, pluginInstance, &world->map,
    //             dir, dir, dir, dir,
    //             get_port_value, this, LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE, nullptr);


    // printf("\n");
}

void PluginAdapter::update_port(ParamPort* port, float float_val) {
    int zzub_val = port->lilv_to_zzub_value(float_val);
    assert(ev.size == sizeof(float));
    values[port->paramIndex] = float_val;
    port->putData((uint8_t*) global_values, zzub_val);
    _host->control_change(metaPlugin, 1, 0, port->paramIndex, zzub_val, false, true);
}


void PluginAdapter::process_events() {
    uint8_t* globals = (u_int8_t*) global_values;
    int value = 0;
    for (ParamPort *port: info->paramPorts) {
        switch(port->zzubParam->type) {
        case zzub::parameter_type_word:
            value = *((unsigned short*) globals);
            break;
        case zzub::parameter_type_switch:
            value = *((unsigned char*) globals);
            break;
        case zzub::parameter_type_note:
        case zzub::parameter_type_byte:
            value = *((unsigned char*) globals);
            break;
        }

        globals += port->byteSize;
        
        if (value != port->zzubParam->value_none) {
            values[port->paramIndex] = port->zzub_to_lilv_value(value);
        }
        
    }
    // Don't zero eventcount here: it could have been
    // incremented in midi_note or set_track_count.
    // Instead, zero it in process_stereo after processing
    // events.
    for (int t = 0; t < trackCount; t++) {
        tvals &vals = track_vals[t];
        tvals &state = track_states[t];

        if (vals.volume != TRACKVAL_VOLUME_UNDEFINED)
            state.volume = vals.volume;

        if (vals.note == zzub::note_value_none) {
            if(state.note != zzub::note_value_none) {
                // midiEvents.aftertouch(attrVals.channel, vals.note, volume);
            }
        } else if(vals.note != zzub::note_value_off) {
            midiEvents.noteOn(attr_vals.channel, vals.note, state.volume);
            state.note = vals.note;
        } else if(state.note != zzub::note_value_none) {
            midiEvents.noteOff(attr_vals.channel, state.note);
            state.note = zzub::note_value_none;
        }

        process_track_midi_events(vals.msg_1, state.msg_1);
        process_track_midi_events(vals.msg_2, state.msg_2);
    }
}

void PluginAdapter::process_track_midi_events(midi_msg &vals_msg, midi_msg& state_msg) {
    if(vals_msg.midi.cmd != TRACKVAL_NO_MIDI_CMD) {
        state_msg.midi.cmd = vals_msg.midi.cmd;
        if (vals_msg.midi.data != TRACKVAL_NO_MIDI_DATA)
            state_msg.midi.data = vals_msg.midi.data;
        midiEvents.add_message(state_msg);
    }
}



// inline LV2_Atom* populatePosBuf(PluginWorld *world, LV2_Atom_Forge *forge, PlaybackPosition &info) {
//     static uint8_t pos_buf[256];
//     /* Build an LV2 position object to report change to plugin */
//     lv2_atom_forge_set_buffer(forge, pos_buf, sizeof(pos_buf));
//     LV2_Atom_Forge_Frame frame;
//     lv2_atom_forge_object(forge, &frame, 0, world->urids.time_Position);

//     lv2_atom_forge_key(forge, world->urids.time_frame);
// //    lv2_atom_forge_long(forge, pos.frame);
//     lv2_atom_forge_long(forge, info.workPos);

//     lv2_atom_forge_key(forge, world->urids.time_speed);
//     lv2_atom_forge_float(forge, info.playing);
// //    if (pos.valid & JackPositionBBT) {
//     lv2_atom_forge_key(forge, world->urids.time_barBeat);
//     lv2_atom_forge_float(forge, info.barBeat);

//     lv2_atom_forge_key(forge, world->urids.time_bar);
//     lv2_atom_forge_long(forge, (uint32_t) info.bar);

//     lv2_atom_forge_key(forge, world->urids.time_beatUnit);
//     lv2_atom_forge_int(forge, info.ticksPerBeat);

//     lv2_atom_forge_key(forge, world->urids.time_beatsPerBar);
//     lv2_atom_forge_float(forge, info.beatsPerBar);

//     lv2_atom_forge_key(forge, world->urids.time_beatsPerMinute);
//     lv2_atom_forge_float(forge, info.beatsPerMinute);
// //    }

//     return (LV2_Atom*) pos_buf;
// }



bool PluginAdapter::process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }

bool PluginAdapter::process_stereo(float **pin, float **pout, int numsamples, int const mode) {
    if (mode == zzub::process_mode_no_io) {
        // printf("mode no io process_stereo() returning false\n");
        return false;
    }

    for(EventPort *port: info->midiPorts) {
        if(port->flow != PortFlow::Input) {
            continue;
        }

        lv2_evbuf_reset(midiBufs[port->bufIndex], true);
        // lv2_atom_sequence_clear(midiSeqs[port->bufIndex]);

        if(midiEvents.count() == 0) {
            continue;
        }

        LV2_Evbuf_Iterator buf_iter = lv2_evbuf_begin(midiBufs[port->bufIndex]);
        // LV2_Evbuf_Iterator buf_iter = lv2_evbuf_begin(midiSeqs[port->bufIndex]);

        for (auto& midi_event: midiEvents.data) {
            // printf("%s\n", midi_event.str().c_str());
            // midi_event.event.body.type = world->urids.midi_MidiEvent;
            // lv2_atom_sequence_append_event(midiSeqs[port->bufIndex], world->hostParams.bufSize, &midi_event.event);
            lv2_evbuf_write(&buf_iter, 
                            midi_event.time, 0, 
                            world->urids.midi_MidiEvent, 
                            midi_event.size, 
                            midi_event.data);
        }
        // printf("reset\n");
        midiEvents.reset();
    }

    samp_count += numsamples;
    if(samp_count - last_update > update_every) {
        apply_events_from_ui();
        last_update = samp_count;
    }

    float *tmp_sampbuf;

    switch(info->audio_in_count) {
    case 0:    // No audio inputs
        break;

    case 1:
        tmp_sampbuf = audioBufs;
        for (int i = 0; i < numsamples; i++) {
            *tmp_sampbuf++ = (pin[0][i] + pin[1][i]) * 0.5f;
        }
        break;

    case 2:
    default:   // FIXME if it's greater than 2, should mix some of them.
        memcpy(audioIn, pout[0], sizeof(float) * numsamples);
        memcpy(audioIn+ZZUB_BUFLEN, pout[1], sizeof(float) * numsamples);
        break;
    }

    // FIXME midi -> lv2_evbuf in process_events
    lilv_instance_run(pluginInstance, numsamples);
    //    if (desc->run_synth) {
    //        desc->run_synth(handle, numsamples, events, eventcount);
    //    } else if (desc->run_multiple_synths) {
    //        desc->run_multiple_synths(1, &handle, numsamples, &events, &eventcount);
    //    } else {
    //    // No synth method -- what should we return?
    //    }

    switch(info->audio_out_count) {
    case 0:
        return true;
    case 1:
        memcpy(pout[0], audioOut, sizeof(float) * numsamples);
        memcpy(pout[1], audioOut, sizeof(float) * numsamples);
        return true;
    case 2:
    default:
        memcpy(pout[0], audioOut, sizeof(float) * numsamples);
        memcpy(pout[1], audioOut+ZZUB_BUFLEN, sizeof(float) * numsamples);
        return true;
    }

}

// void PluginAdapter::midi_note(int channel, int value, int velocity)
// {
//     if (verbose) {
//         printf("midi_note: %d %d %d\n", channel, value, velocity);
//     }

//     midiEvents.noteOn(channel, value, velocity);
    
//     return;
// }

// void PluginAdapter::midi_note_off(int channel, int value, int velocity) {
//     if (verbose) {
//         printf("midi_note_off: %d %d %d\n", channel, value, velocity);
//     }

//     midiEvents.noteOff(channel, value);
    
//     return;
// }



// void PluginAdapter::process_controller_events() {}
// void PluginAdapter::attributes_changed() {}
// void PluginAdapter::command(int) {}
// void PluginAdapter::mute_track(int) {}
// bool PluginAdapter::is_track_muted(int) const { return false; }
// void PluginAdapter::event(unsigned int) {}
// const zzub::envelope_info** PluginAdapter::get_envelope_infos() { return 0; }
// bool PluginAdapter::play_wave(int, int, float) { return false; }
// void PluginAdapter::stop_wave() {}
// int PluginAdapter::get_wave_envelope_play_position(int) { return -1; }
// const char* PluginAdapter::describe_param(int) { return 0; }
// bool PluginAdapter::set_instrument(const char*) { return false; }
// void PluginAdapter::get_sub_menu(int, zzub::outstream*) {}
// void PluginAdapter::add_input(const char*) {}
// void PluginAdapter::delete_input(const char*) {}
// void PluginAdapter::rename_input(const char*, const char*) {}
// void PluginAdapter::input(float**, int, float) {}
// void PluginAdapter::midi_control_change(int, int, int) {}
// bool PluginAdapter::handle_input(int, int, int) { return false; }




zzub::plugin *PluginInfo::create_plugin() const {
    return new PluginAdapter((PluginInfo*) &(*this));
}

struct lv2plugincollection : zzub::plugincollection {
    PluginWorld *world = PluginWorld::getInstance();
   
    virtual void initialize(zzub::pluginfactory *factory) {
        const LilvPlugins* const collection = world->get_all_plugins();
        LILV_FOREACH(plugins, iter, collection) {
            const LilvPlugin *plugin = lilv_plugins_get(collection, iter);
            PluginInfo *info = new PluginInfo(world, plugin);
            factory->register_info(info);
        }
    }

    // Called by the host upon song loading. If the collection
    // can not provide a plugin info based on the uri or
    // the metainfo passed, it should return a null pointer.
    virtual const zzub::info *get_info(const char *uri, zzub::archive *data) { return 0; }

    // Returns the uri of the collection to be identified,
    // return zero for no uri. Collections without uri can not be
    // configured.
    virtual const char *get_uri() { return 0; }

    // Called by the host to set specific configuration options,
    // usually related to paths.
    virtual void configure(const char *key, const char *value) {}

    // Called by the host upon destruction. You should
    // delete the instance in this function
    virtual void destroy() {
        delete this;
    }
};


zzub::plugincollection *zzub_get_plugincollection() {
    return new lv2plugincollection();
}



const char *zzub_get_signature() { return ZZUB_SIGNATURE; }


void write_events_from_ui(void* const adapter_handle,
                  uint32_t    port_index,
                  uint32_t    buffer_size,
                  uint32_t    protocol,
                  const void* buffer) {
    PluginAdapter* const adapter = (PluginAdapter *) adapter_handle;

	if (protocol != 0 && protocol != adapter->world->urids.atom_eventTransfer) {
		fprintf(stderr, "UI write with unsupported protocol %u (%s)\n", protocol, unmap_uri(adapter->world, protocol));
		return;
	}

   	if (port_index >= adapter->info->ports.size()) {
		fprintf(stderr, "UI write to out of range port index %u\n", port_index);
		return;
	}

	char buf[sizeof(ControlChange) + buffer_size];
	ControlChange* ev = (ControlChange*)buf;
	ev->index    = port_index;
	ev->protocol = protocol;
	ev->size     = buffer_size;
	memcpy(ev->body, buffer, buffer_size);
	zix_ring_write(adapter->ui_events, buf, sizeof(buf));
}


uint32_t lv2_port_index(void* const adapter_handle, const char* symbol) { 
    PluginAdapter* const adapter = (PluginAdapter *) adapter_handle;

    Port* port = adapter->info->port_by_symbol(symbol);

    return port ? port->index : LV2UI_INVALID_PORT_INDEX;
}


const void* get_port_value(const char* port_symbol,
                           void*       user_data,
                           uint32_t*   size,
                           uint32_t*   type)
{
    PluginAdapter* adapter = (PluginAdapter*) user_data;
    ParamPort* port = adapter->get_param_port(port_symbol);

    if (port != nullptr) {
        *size = sizeof(float);
        *type = adapter->world->forge.Float;
        return &adapter->values[port->paramIndex];
    }

    *size = *type = 0;
    return NULL;
}


void set_port_value(const char* port_symbol,
                    void*       user_data,
                    const void* value,
                    uint32_t    size,
                    uint32_t    type)
{
    // if(verbose)
    printf("SET PORT VALUE %s\n", port_symbol);
    PluginAdapter* adapter = (PluginAdapter*) user_data;
    //			                                                   port->lilv_port);
    ParamPort* port = adapter->get_param_port(port_symbol);

    if (port == nullptr) {
        fprintf(stderr, "error: Preset port `%s' is missing\n", port_symbol);
        return;
    }

    

    float fvalue;
    if (type == adapter->world->forge.Float) {
        fvalue = *(const float*)value;
    } else if (type == adapter->world->forge.Double) {
        fvalue = *(const double*)value;
    } else if (type == adapter->world->forge.Int) {
        fvalue = *(const int32_t*)value;
    } else if (type == adapter->world->forge.Long) {
        fvalue = *(const int64_t*)value;
    } else {
        fprintf(stderr, "error: Preset `%s' value has bad type <%s>\n",
                port_symbol, adapter->world->unmap.unmap(adapter->world->unmap.handle, type));
        return;
    }

    adapter->values[port->paramIndex] = fvalue;

    //	if (jalv->play_state != JALV_RUNNING) {
    //		// Set value on port struct directly
    //		port->control = fvalue;
    //	} else {
    //		// Send value to running plugin
    //		jalv_ui_write(jalv, port->index, sizeof(fvalue), 0, &fvalue);
    //	}

    	if (adapter->ui_instance) {
    		// Update UI
    		char buf[sizeof(ControlChange) + sizeof(fvalue)];
    		ControlChange* ev = (ControlChange*)buf;
    		ev->index    = port->index;
    		ev->protocol = 0;
    		ev->size     = sizeof(fvalue);
    		*(float*)ev->body = fvalue;
    		zix_ring_write(adapter->plugin_events, buf, sizeof(buf));
    	}
}
