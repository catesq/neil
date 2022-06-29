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

// lv2 plugin adapter by github.com/mitnuh
// based on the dssi adapter by
//     Copyright (C) 2006 Leonard Ritter (contact@leonard-ritter.com)
//     Copyright (C) 2008 James McDermott (jamesmichaelmcdermott@gmail.com)

#include "lv2_defines.h"
#include "lv2_utils.h"
#include "PluginAdapter.h"
#include "PluginInfo.h"
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

#include <thread>
#include <cstdlib>
#include "X11/Xlib.h"

#include <iostream>

//const char *zzub_adapter_name = "zzub lv2 adapter";




PluginAdapter::PluginAdapter(PluginInfo *info) : info(info), cache(info->cache) {
    if (verbose) { printf("plugin <%s>: in constructor. data size: %d\n", info->name.c_str(), info->zzubTotalDataSize); }

    if(info->zzubTotalDataSize) {
        global_values = malloc(info->zzubTotalDataSize);
        memset(global_values, 0, info->zzubTotalDataSize);
    }

    uis           = lilv_plugin_get_uis(info->lilvPlugin);
    ui_events     = zix_ring_new(EVENT_BUF_SIZE);
    plugin_events = zix_ring_new(EVENT_BUF_SIZE);

    track_values = trak_values;
    attributes   = (int *) &attr_values;

    for(Port* port: info->ports) {
        switch(port->type) {
        case PortType::Control:
            controlPorts.push_back(new ControlPort(*static_cast<ControlPort*>(port)));
            ports.push_back(controlPorts.back());
            break;


        case PortType::Param:
            paramPorts.push_back(new ParamPort(*static_cast<ParamPort*>(port)));
            ports.push_back(paramPorts.back());
            break;

        case PortType::Audio:
            if(port->flow == PortFlow::Input) {
                audioInPorts.push_back(new AudioBufPort(*static_cast<AudioBufPort*>(port)));
                audioInPorts.back()->buf = (float*) malloc(sizeof(float) * ZZUB_BUFLEN);
                ports.push_back(audioInPorts.back());
            } else if (port->flow == PortFlow::Output) {
                audioOutPorts.push_back(new AudioBufPort(*static_cast<AudioBufPort*>(port)));
                audioOutPorts.back()->buf = (float*) malloc(sizeof(float) * ZZUB_BUFLEN);
                ports.push_back(audioOutPorts.back());
            }
            break;


        case PortType::CV:
            cvPorts.push_back(new AudioBufPort(*static_cast<AudioBufPort*>(port)));
            cvPorts.back()->buf = (float*) malloc(sizeof(float) * ZZUB_BUFLEN);
            ports.push_back(cvPorts.back());
            break;

        case PortType::Event:
            eventPorts.push_back(new EventBufPort(*static_cast<EventBufPort*>(port)));
            eventPorts.back()->eventBuf = lv2_evbuf_new(
                is_distrho_event_out_port(port) ? cache->hostParams.paddedBufSize : cache->hostParams.bufSize,
                cache->urids.atom_Chunk,
                cache->urids.atom_Sequence
            );
            ports.push_back(eventPorts.back());
            break;


        case PortType::Midi:
            midiPorts.push_back(new EventBufPort(*static_cast<EventBufPort*>(port)));
            midiPorts.back()->eventBuf = lv2_evbuf_new(cache->hostParams.bufSize, cache->urids.atom_Chunk, cache->urids.atom_Sequence);
            ports.push_back(midiPorts.back());
            break;

        default:
            ports.push_back(new Port(*port));
            break;
        }
    }

    if(verbose) { printf("Construted buffers. audio & cv buflen: %u, event buflen: %i.\n", ZZUB_BUFLEN, cache->hostParams.bufSize); }
}

PluginAdapter::~PluginAdapter() {
    halting = true;
    lv2_worker_finish(&this->worker);

    if(this->suil_ui_instance)
        this->ui_destroy();

    zix_ring_free(ui_events);
    zix_ring_free(plugin_events);
    zix_sem_destroy(&worker.sem);

    for(auto port: ports)
        delete port;

    if (lilvInstance != nullptr)
        lilv_instance_free(lilvInstance);

    lilv_uis_free(uis);

    free(global_values);
}


void
PluginAdapter::init(zzub::archive *arc) {
    bool use_show_ui = false;
    sample_rate      = _master_info->samples_per_second;
    ui_scale         = gtk_widget_get_scale_factor((GtkWidget*) _host->get_host_info()->host_ptr);

    LV2_Options_Option options[] = {
        {
             LV2_OPTIONS_INSTANCE, 0,
             cache->urids.param_sampleRate,
             sizeof(float), cache->urids.atom_Float,
             &sample_rate
        },
        {
             LV2_OPTIONS_INSTANCE, 0,
             cache->urids.minBlockLength,
             sizeof(int32_t), cache->urids.atom_Int,
             &cache->hostParams.minBlockLength
        },
        {
             LV2_OPTIONS_INSTANCE, 0,
             cache->urids.maxBlockLength,
             sizeof(int32_t), cache->urids.atom_Int,
             &cache->hostParams.blockLength
        },
        {
             LV2_OPTIONS_INSTANCE, 0,
             cache->urids.bufSeqSize,
             sizeof(int32_t), cache->urids.atom_Int,
             &cache->hostParams.bufSize
        },
        {
             LV2_OPTIONS_INSTANCE, 0,
             cache->urids.ui_updateRate,
             sizeof (float), cache->urids.atom_Float,
             &update_rate
        },
        {
             LV2_OPTIONS_INSTANCE, 0,
             cache->urids.ui_scaleFactor,
             sizeof (float), cache->urids.atom_Float,
             &ui_scale
        },
        {
             LV2_OPTIONS_INSTANCE, 0,
             cache->urids.ui_transientWindowId,
             sizeof (long), cache->urids.atom_Long,
             &transient_wid
        },
        {
             LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, NULL
        }
    };

    features.options = malloc(sizeof(options));
    memcpy(features.options, options, sizeof(options));

    features.map_feature                   = { LV2_URID__map,                    &cache->map };
    features.unmap_feature                 = { LV2_URID__unmap,                  &cache->unmap };
    features.bounded_buf_feature           = { LV2_BUF_SIZE__boundedBlockLength, &cache->hostParams.blockLength };
    features.make_path_feature             = { LV2_STATE__makePath,              &cache->make_path };
    features.options_feature               = { LV2_OPTIONS__options,             features.options};
    features.program_host_feature          = { LV2_PROGRAMS__Host,               &features.program_host };
    features.data_access_feature           = { LV2_DATA_ACCESS_URI,              NULL };
    features.worker_feature                = { LV2_WORKER__schedule,             &features.worker_schedule };

    features.ui_parent_feature             = { LV2_UI__parent  ,                 NULL};
    features.ui_instance_feature           = { LV2_INSTANCE_ACCESS_URI,          NULL };
    features.ui_idle_feature               = { LV2_UI__idleInterface,            NULL };
    features.ui_data_access_feature        = { LV2_DATA_ACCESS_URI,              &features.ext_data };

    features.program_host.handle           = this;
    features.program_host.program_changed  = &program_changed;

    worker.plugin                          = this;
    features.worker_schedule.handle        = &worker;
    features.worker_schedule.schedule_work = &lv2_worker_schedule;

    const LV2_Feature* feature_list[] = {
        &features.map_feature,
        &features.unmap_feature,
        &features.program_host_feature,
        &features.options_feature,
        &features.bounded_buf_feature,
        &features.data_access_feature,
        &features.worker_feature,
        NULL
    };

    zix_sem_init(&worker.sem, 0);
    zix_sem_init(&work_lock, 1);

    lilvInstance                      = lilv_plugin_instantiate(info->lilvPlugin, _master_info->samples_per_second, feature_list);

    features.ext_data.data_access     = lilv_instance_get_descriptor(lilvInstance)->extension_data;

    features.ui_instance_feature.data = lilv_instance_get_handle(lilvInstance);

    worker.enable                     = lilv_plugin_has_extension_data(info->lilvPlugin, cache->nodes.worker_iface);

    metaPlugin                        = _host->get_metaplugin();
    _host->set_event_handler(metaPlugin, this);

    if (worker.enable) {
        auto iface = lilv_instance_get_extension_data (lilvInstance, LV2_WORKER__interface);
        lv2_worker_init(this, &worker, (const LV2_Worker_Interface*) iface, true);
    }

    connect(lilvInstance);
    lilv_instance_activate(lilvInstance);


    if(arc == nullptr)
        return;

    auto* instream = arc->get_instream("");

    if(instream == nullptr)
        return;

    if(instream->size() > 4) {
        uint32_t state_len;
        instream->read(state_len);
        char* state_str        = (char*) malloc(state_len + 1);
        instream->read(state_str, state_len);
        state_str[state_len]   = 0;
        LilvState* lilvState   = lilv_state_new_from_string(cache->lilvWorld, &cache->map, state_str);
        lilv_state_restore(lilvState, lilvInstance, &set_port_value, this, LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE, nullptr);
    }


}



extern "C" {
    void on_window_destroy(GtkWidget* widget, gpointer data) {
       static_cast<PluginAdapter*>(data)->ui_destroy();
    }
}

bool PluginAdapter::invoke(zzub_event_data_t& data) {
    if (data.type != zzub::event_type_double_click || !(info->flags & zzub_plugin_flag_has_custom_gui))
        return false;

    if(suil_ui_host == nullptr) {
        return ui_open();
    } else {
        printf("already hosted\n");
        return false;
    }
}



void program_changed(LV2_Programs_Handle handle, int32_t index) {
    // printf("program changed\n");
    printf("PluginAdapter::program_changed\n");

    PluginAdapter* adapter = (PluginAdapter*) handle;
    adapter->update_all_from_ui();
    adapter->program_change_update = true;
}


void PluginAdapter::destroy() {
    delete this;
}



void PluginAdapter::connect(LilvInstance* pluginInstance) {
    if(verbose) { printf("in connectInstance\n"); }

    uint8_t* tmp_globals_p = (uint8_t*) global_values;
    for(auto& paramPort: paramPorts) {
        switch(paramPort->zzubParam.type) {
            case zzub::parameter_type_note:
            case zzub::parameter_type_byte:
            case zzub::parameter_type_switch:
                *tmp_globals_p = (uint8_t) paramPort->zzubParam.value_default;
                break;
            case zzub::parameter_type_word:
                *((uint16_t*)tmp_globals_p) = (uint16_t) paramPort->zzubParam.value_default;
                break;
        }
        
        tmp_globals_p += paramPort->zzubValSize;
    }

    for(auto port: ports)
        lilv_instance_connect_port(pluginInstance, port->index, port->data_pointer());
}



void PluginAdapter::load(zzub::archive *arc) {
    // This is called when user selects a new preset in Aldrin.
    if (verbose) printf("PluginAdapter: in load preset()!\n");
    printf("load saved song settings for %s\n", info->name.c_str());
    printf("load ing\n");
}


void PluginAdapter::save(zzub::archive *arc) {
    if (verbose) printf("PluginAdapter: in save()!\n");
    const char *dir = cache->hostParams.tempDir.c_str();
    LilvState* const state = lilv_state_new_from_instance(
                info->lilvPlugin, lilvInstance, &cache->map,
                dir, dir, dir, dir,
                &get_port_value, this, LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE, nullptr);

    char *state_str = lilv_state_to_string(cache->lilvWorld, &cache->map, &cache->unmap, state, "http://uri", nullptr);
    zzub::outstream *po = arc->get_outstream("");
    po->write((uint32_t) strlen(state_str));
    po->write(state_str, strlen(state_str));
}


const char *PluginAdapter::describe_value(int param, int value) {
    static char text[256];
    if(param < paramPorts.size()) {
        return paramPorts[param]->describeValue(value, text);
    }
    return 0;
}

void PluginAdapter::set_track_count(int ntracks) {
    if (ntracks < trackCount) {
        for (int t = ntracks; t < trackCount; t++) {
            if (trak_states[t].note != zzub::note_value_none) {
                midiEvents.noteOff(t, trak_states[t].note, 0);
            }
        }
    }

    trackCount = ntracks;
}

ParamPort* PluginAdapter::get_param_port(std::string symbol) {
    for(auto& port: paramPorts)
        if(symbol == port->symbol)
            return port;

    return nullptr;
}

void PluginAdapter::stop() {}



void PluginAdapter::update_port(ParamPort* port, float float_val) {
    printf("Update port: index=%d, name='%s', value=%f\n", port->index, port->name.c_str(), float_val);
//    int zzub_val = port->lilv_to_zzub_value(float_val);
//    values[port->paramIndex] = float_val;
//    port->putData((uint8_t*) global_values, zzub_val);
//    _host->control_change(metaPlugin, 1, 0, port->paramIndex, zzub_val, false, true);
}


void PluginAdapter::process_events() {
    if(halting)
        return;

    if(show_interface != nullptr && suil_ui_handle != nullptr) {
        if(!showing_interface) {
            printf("Show feature for %s\n", info->name.c_str());
            (show_interface->show)(suil_ui_handle);
            showing_interface = true;
        }

        if(showing_interface && idle_interface != nullptr) {
//            printf("Run idle feature for %s\n", info->name.c_str());
            (idle_interface->idle)(suil_ui_handle);
        }

    }

    uint8_t* globals = (u_int8_t*) global_values;
    int value = 0;

    for (auto &paramPort: paramPorts) {
        switch(paramPort->zzubParam.type) {
        case zzub::parameter_type_word:
            value = *((unsigned short*) globals);
            break;
        case zzub::parameter_type_switch:
        case zzub::parameter_type_note:
        case zzub::parameter_type_byte:
            value = *((unsigned char*) globals);
            break;
        }

        globals += paramPort->zzubValSize;
        
        if (value != paramPort->zzubParam.value_none) {
            paramPort->value = paramPort->zzub_to_lilv_value(value);
        }
    }


    if(info->flags & zzub_plugin_flag_is_instrument)
        process_all_midi_tracks();
}

void PluginAdapter::process_all_midi_tracks() {
    for (int t = 0; t < trackCount; t++) {
        if (trak_values[t].volume != TRACKVAL_VOLUME_UNDEFINED)
            trak_states[t].volume = trak_values[t].volume;

        if (trak_values[t].note == zzub::note_value_none) {

            if(trak_values[t].volume != TRACKVAL_VOLUME_UNDEFINED && trak_states[t].note != zzub::note_value_none) {
                midiEvents.aftertouch(attr_values.channel, trak_states[t].note, trak_states[t].volume);
            }

        } else if(trak_values[t].note != zzub::note_value_off) {
            midiEvents.noteOn(attr_values.channel, trak_values[t].note, trak_states[t].volume);
            trak_states[t].note = trak_values[t].note;

        } else if(trak_states[t].note != zzub::note_value_none) {

            midiEvents.noteOff(attr_values.channel, trak_states[t].note);
            // this is wrong but some synths glitch when an aftertouch is sent after a note off
            // it relies on state.note being a valid note.
            // if a note off with volume 0 is set then clear state.note to prevent aftertouch
            if(trak_values[t].volume == 0)
                trak_states[t].note = zzub::note_value_none;
        }

        process_one_midi_track(trak_values[t].msg_1, trak_states[t].msg_1);
        process_one_midi_track(trak_values[t].msg_2, trak_states[t].msg_2);
    }

    for(EventBufPort* midiPort: midiPorts) {
        if(midiPort->flow != PortFlow::Input || midiEvents.count() == 0)
            continue;

        LV2_Evbuf_Iterator buf_iter = lv2_evbuf_begin(midiPort->eventBuf);

        for (auto& midi_event: midiEvents.data)
            lv2_evbuf_write(&buf_iter,
                            midi_event.time, 0,
                            cache->urids.midi_MidiEvent,
                            midi_event.size,
                            midi_event.data);
    }

    midiEvents.reset();
}



void PluginAdapter::process_one_midi_track(midi_msg &vals_msg, midi_msg& state_msg) {
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
    if (halting || mode == zzub::process_mode_no_io)
        return false;

    samp_count += numsamples;

    for(EventBufPort* eventPort: eventPorts)
        if(eventPort->flow == PortFlow::Output)
            lv2_evbuf_reset(eventPort->eventBuf, false);

    if(samp_count - last_update > update_every) {
        ui_event_import();
        ui_event_dispatch();
        last_update = samp_count;
    }


    switch(audioInPorts.size()) {
    case 0:    // No audio inputs
        break;

    case 1: {
        float *sample = audioInPorts[0]->buf;
        for (int i = 0; i < numsamples; i++)
            *sample++ = (pin[0][i] + pin[1][i]) * 0.5f;
        break;
    }

    case 2:
    default:   // FIXME if it's greater than 2, should mix some of them.
        memcpy(audioInPorts[0]->buf, pout[0], sizeof(float) * numsamples);
        memcpy(audioInPorts[1]->buf, pout[1], sizeof(float) * numsamples);
        break;
    }

    lilv_instance_run(lilvInstance, numsamples);

    /* Process any worker replies. */
    if(worker.enable) {
        lv2_worker_emit_responses(&worker, lilvInstance);

        /* Notify the plugin the run() cycle is finished */
        if (worker.iface && worker.iface->end_run) {
            worker.iface->end_run(lilvInstance->lv2_handle);
        }
    }


    for(EventBufPort* port: midiPorts)
        if(port->flow == PortFlow::Input)
            lv2_evbuf_reset(port->eventBuf, true);


    switch(audioOutPorts.size()) {
    case 0:
        return true;

    case 1:
        memcpy(pout[0], audioOutPorts[0]->buf, sizeof(float) * numsamples);
        memcpy(pout[1], audioOutPorts[0]->buf, sizeof(float) * numsamples);
        return true;

    case 2:
    default:
        memcpy(pout[0], audioOutPorts[0]->buf, sizeof(float) * numsamples);
        memcpy(pout[1], audioOutPorts[1]->buf, sizeof(float) * numsamples);
        return true;
    }
}


struct lv2plugincollection : zzub::plugincollection {
    SharedCache *world = SharedCache::getInstance();
   
    virtual void initialize(zzub::pluginfactory *factory) {
        const LilvPlugins* const collection = world->get_all_plugins();
        LILV_FOREACH(plugins, iter, collection) {
            const LilvPlugin *plugin = lilv_plugins_get(collection, iter);
            PluginInfo *info = new PluginInfo(world, plugin);
            factory->register_info(info);
        }
    }

    virtual const zzub::info *get_info(const char *uri, zzub::archive *data) { return 0; }

    virtual const char *get_uri() { return 0; }

    virtual void configure(const char *key, const char *value) {}

    virtual void destroy() {
        delete this;
    }
};


zzub::plugincollection *zzub_get_plugincollection() {
    return new lv2plugincollection();
}



const char *zzub_get_signature() { return ZZUB_SIGNATURE; }

