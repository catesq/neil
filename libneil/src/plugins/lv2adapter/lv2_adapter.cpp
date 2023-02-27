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

#include "zzub/plugin.h"

#include "lv2/state/state.h"
#include "lv2/options/options.h"
#include "lv2/instance-access/instance-access.h"
#include "lv2/buf-size/buf-size.h"

#include "lv2_defines.h"
#include "lv2_utils.h"
#include "lv2_adapter.h"
#include "lv2_zzub_info.h"

#include <gdk/gdk.h>
#include <glib-object.h>
#include <glib.h>
#include <gobject/gclosure.h>
#include <gtk/gtk.h>

#include <ostream>
#include <string>
#include <thread>
#include <cstdlib>
#include <iostream>


lv2_adapter::lv2_adapter(lv2_zzub_info *info) 
  : info(info), 
    cache(info->cache), 
    midi_track_manager(*this, NUM_TRACKS)
{
    
    if(info->zzubTotalDataSize) 
    {
        global_values = malloc(info->zzubTotalDataSize);
        memset(global_values, 0, info->zzubTotalDataSize);
    }

    uis = lilv_plugin_get_uis(info->lilvPlugin);
    ui_events = zix_ring_new(EVENT_BUF_SIZE);
    plugin_events = zix_ring_new(EVENT_BUF_SIZE);
    attributes   = (int *) &attr_values;

    track_values = malloc(sizeof(struct zzub::midi_note_track) * NUM_TRACKS);

    init_ports();

    if(verbose) { printf("Construted buffers. audio & cv buflen: %u, event buflen: %i.\n", ZZUB_BUFLEN, cache->hostParams.bufSize); }
}

lv2_adapter::~lv2_adapter() 
{
    halting = true;
    lv2_worker_finish(&this->worker);

    if(this->suil_ui_instance)
        this->ui_destroy();

    zix_ring_free(ui_events);
    zix_ring_free(plugin_events);
    zix_sem_destroy(&worker.sem);

    for(auto port: ports) 
        delete port;

    if(lilvInstance != nullptr)
        lilv_instance_free(lilvInstance);

    lilv_uis_free(uis);

    free(global_values);
}


void
lv2_adapter::build_ports()
{
    // build buffers for audio samples, midi events, control value samples etc. 
    // also store each port in a vector for that port type, mainly so it's easier to use them in the process_stereo()
    for(lv2_port* port: info->ports) {
        switch(port->type) {
            case PortType::Control:
            {
                controlPorts.emplace_back(new control_port(*static_cast<control_port*>(port)));
                ports.push_back(controlPorts.back());
                break;
            }

            case PortType::Param: 
            {
                auto par_port = new param_port(*static_cast<param_port*>(port));
                par_port->set_value(par_port->defaultValue);

                paramPorts.emplace_back(par_port);
                ports.push_back(par_port);
                break;
            }

            case PortType::Audio: 
            {
                auto audio_port = new audio_buf_port(*static_cast<audio_buf_port*>(port));
                audio_port->set_buffer((float*) malloc(sizeof(float) * ZZUB_BUFLEN));
                memset(audio_port->data_pointer(), 0, sizeof(float) * ZZUB_BUFLEN);

                if(port->flow == PortFlow::Input) {
                    audioInPorts.push_back(audio_port);
                } else if (port->flow == PortFlow::Output) {
                    audioOutPorts.push_back(audio_port);
                }

                ports.push_back(audio_port);
                break;
            }

            case PortType::CV:
            {
                auto cv_port = new audio_buf_port(*static_cast<audio_buf_port*>(port));

                cv_port->set_buffer((float*) malloc(sizeof(float) * ZZUB_BUFLEN));

                memset(cv_port->data_pointer(), 0, sizeof(float) * ZZUB_BUFLEN);

                cvPorts.push_back(cv_port);
                ports.push_back(cv_port);
                break;
            }

            case PortType::Event:
            {
                auto event_port = new event_buf_port(*static_cast<event_buf_port*>(port));

                auto buf_size = is_distrho_event_out_port(port) ? cache->hostParams.paddedBufSize : cache->hostParams.bufSize;

                auto event_buf = lv2_evbuf_new(buf_size, cache->urids.atom_Chunk, cache->urids.atom_Sequence);
                
                event_port->set_buffer(event_buf); 

                eventPorts.push_back(event_port);
                ports.push_back(event_port);
                break;
            }

            case PortType::Midi:
            {
                auto midi_port = new event_buf_port(*static_cast<event_buf_port*>(port));
                auto event_buf = lv2_evbuf_new(cache->hostParams.bufSize, cache->urids.atom_Chunk, cache->urids.atom_Sequence);

                midi_port->set_buffer(event_buf);
                
                midiPorts.push_back(midi_port);
                ports.push_back(midi_port);
                break;
            }

            default:
            {
                ports.push_back(new lv2_port(*port));
                break;
            }
        }
    }
}

void 
lv2_adapter::add_note_on(uint8_t note, uint8_t volume) {
    midiEvents.noteOn(0, note, volume);
}

void 
lv2_adapter::add_note_off(uint8_t note) {
    midiEvents.noteOff(0, note);
}

void 
lv2_adapter::add_aftertouch(uint8_t note, uint8_t volume) {
    midiEvents.aftertouch(0, note, volume);
}

void 
lv2_adapter::add_midi_command(uint8_t cmd, uint8_t data1, uint8_t data2) {
    midiEvents.add(0, cmd, data1, data2);
}

zzub::midi_note_track* 
lv2_adapter::get_track_data_pointer(uint16_t track_num) const {
    return &((zzub::midi_note_track*) track_values)[track_num];
}



void
lv2_adapter::init(zzub::archive *arc) 
{
    sample_rate = _master_info->samples_per_second;
    ui_scale = gtk_widget_get_scale_factor((GtkWidget*) _host->get_host_info()->host_ptr);

    midi_track_manager.init(sample_rate);

    LV2_Options_Option options[] = 
    {
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

    const LV2_Feature* feature_list[] = 
    {
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

    if (worker.enable) 
    {
        auto iface = lilv_instance_get_extension_data (lilvInstance, LV2_WORKER__interface);
        lv2_worker_init(this, &worker, (const LV2_Worker_Interface*) iface, true);
    }


    for(auto& port: ports)
        lilv_instance_connect_port(lilvInstance, port->index, port->data_pointer());

    lilv_instance_activate(lilvInstance);

    if(arc == nullptr)
        return;

    auto* instream = arc->get_instream("");

    if(!instream)
        return;

    uint32_t arc_type = 0;
    instream->read(arc_type);

    // arc_type is either a marker that indicates the save state was 
    //   a list of floating point values stored/restored by the lvadapter
    //   an opaque blob handled by the plugin the adapter is proxying
    if(arc_type == ARCHIVE_USES_PARAMS)
        read_archive_params(instream);
    else
        read_archive_state(instream, arc_type);
}



void 
lv2_adapter::created() 
{
    initialized = true;
    printf("Initialised lv2 adapter\n");
    for(param_port* port: paramPorts) {
        _host->set_parameter(metaPlugin, 1, 0, port->paramIndex, port->lilv_to_zzub_value(port->value));
        // DEBUG_INFO("Helm", "\nSet param %d(%s). lilv val: %f, zzub val: %d\n", port->paramIndex, port->name.c_str(), port->value, port->lilv_to_zzub_value(port->value))
    }
}



bool 
lv2_adapter::invoke(zzub_event_data_t& data) 
{
    if (data.type != zzub::event_type_double_click || (ui_is_open && !ui_is_hidden) || !(info->flags & zzub_plugin_flag_has_custom_gui)) {
        return false;
    } else if(ui_is_hidden) {
        ui_reopen();
    } else {
        ui_open();
    }

    return true;
}



void 
program_changed(LV2_Programs_Handle handle, int32_t index) 
{
    // printf("program changed\n");
    printf("PluginAdapter::program_changed\n");

    lv2_adapter* adapter = (lv2_adapter*) handle;
    adapter->update_all_from_ui();
    adapter->program_change = true;
}



void 
lv2_adapter::destroy() 
{
    delete this;
}

void 
lv2_adapter::read_archive_params(zzub::instream* instream) 
{
    printf("PluginAdapter::read_archive_params\n");
    uint32_t param_count = 0;
    instream->read(param_count);

    // DEBUG_INFO("Helm", "store helm param %d: %.2f, params in archive(%d).  params in plugin(%zu)\n", param_port->paramIndex, param_port->value, param_count, paramPorts.size())
    // printf("plugin %s: get %u params from archive, %zu params extected\n", info->name.c_str(), param_count, paramPorts.size());

    if(param_count != paramPorts.size())
        return;

    for(auto& param_port: paramPorts)
        instream->read(param_port->value);
}



void 
lv2_adapter::save_archive_params(zzub::outstream *outstream) 
{
    outstream->write((uint32_t) ARCHIVE_USES_PARAMS);
    outstream->write((uint32_t) paramPorts.size());

    for(auto& param_port: paramPorts)
        outstream->write(param_port->value);
}



void lv2_adapter::read_archive_state(zzub::instream* instream, uint32_t length) 
{
    printf("PluginAdapter::read_archive_state\n");
    char* state_str = (char*) malloc(length + 1);
    instream->read(state_str, length);

    state_str[length] = 0;
    LilvState* lilvState = lilv_state_new_from_string(
        cache->lilvWorld,
        &cache->map,
        state_str
    );

    lilv_state_restore(lilvState, lilvInstance, &set_port_value, this, LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE, nullptr);
}



void 
lv2_adapter::save_archive_state(zzub::outstream *outstream) 
{
    const char *dir = cache->hostParams.tempDir.c_str();

    LilvState* const state = lilv_state_new_from_instance(
        info->lilvPlugin,
        lilvInstance,
        &cache->map,
        dir, dir, dir, dir,
        &get_port_value,
        this,
        LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE,
        nullptr
    );

    char *state_str = lilv_state_to_string(
        cache->lilvWorld, 
        &cache->map, 
        &cache->unmap, 
        state, 
        "http://uri",
        nullptr
    );

    outstream->write((uint32_t) strlen(state_str));
    outstream->write(state_str, strlen(state_str));
}



void 
lv2_adapter::save(zzub::archive *arc) 
{
    if (verbose) 
    printf("PluginAdapter: in save()!\n");

    zzub::outstream *outstream = arc->get_outstream("");
    // if(prefer_state_save()) {
        save_archive_state(outstream);
    // } else {
        // save_archive_params(outstream);
    // }
}


const char *
lv2_adapter::describe_value(int param, int value) 
{
    static char text[256];
    if(param < paramPorts.size())
        return paramPorts[param]->describeValue(value, text);
    return 0;
}


void 
lv2_adapter::set_track_count(int new_num) 
{
    // if (ntracks < trackCount) {
    //     for (int t = ntracks; t < trackCount; t++) {
    //         if (trak_states[t].note != zzub::note_value_none) {
    //             midiEvents.noteOff(t, trak_states[t].note, 0);
    //         }
    //     }
    // }

    trackCount = new_num;
    midi_track_manager.set_track_count(new_num);
}


param_port* 
lv2_adapter::get_param_port(std::string symbol) 
{
    for(auto& port: paramPorts)
        if(symbol == port->symbol)
            return port;

    return nullptr;
}


void 
lv2_adapter::stop() 
{
    
}


void 
lv2_adapter::update_port(param_port* port, float float_val) 
{
    printf("Update port: index=%d, name='%s', value=%f\r", port->paramIndex, port->name.c_str(), float_val);
    port->value = float_val;
    
    //    int zzub_val = port->lilv_to_zzub_value(float_val);
    //    values[port->paramIndex] = float_val;
    //    port->putData((uint8_t*) global_values, zzub_val);
    //    _host->control_change(metaPlugin, 1, 0, port->paramIndex, zzub_val, false, true);
}


void 
lv2_adapter::process_events() 
{
    if(halting || !initialized)
        return;

    //    if(show_interface != nullptr && suil_ui_handle != nullptr) {
    //        if(!showing_interface) {
    //            printf("Show feature for %s\n", info->name.c_str());
    //            (show_interface->show)(suil_ui_handle);
    //            showing_interface = true;
    //        }

    //        if(showing_interface && idle_interface != nullptr) {
    ////            printf("Run idle feature for %s\n", info->name.c_str());
    //            (idle_interface->idle)(suil_ui_handle);
    //        }

    //    }

    uint8_t* globals = (u_int8_t*) global_values;
    int value = 0;

    for (auto &paramPort: paramPorts) 
    {
        switch(paramPort->zzubParam.type) 
        {
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
        
        if (value != paramPort->zzubParam.value_none)
            paramPort->value = paramPort->zzub_to_lilv_value(value);
    }


    // if(info->flags & zzub_plugin_flag_is_instrument)
    //     midi_track_manager.process_midi_events();
        // process_all_midi_tracks();
}



void
lv2_adapter::send_midi_events() 
{
    if(midiEvents.count() == 0)
        return; 

    for(event_buf_port* midiPort: midiPorts) 
    {
        if(midiPort->flow != PortFlow::Input || midiEvents.count() == 0)
            continue;

        LV2_Evbuf_Iterator buf_iter = lv2_evbuf_begin(midiPort->get_lv2_evbuf());

        for (auto& midi_event: midiEvents.data)
        {
            lv2_evbuf_write(&buf_iter,
                            midi_event.time, 0,
                            cache->urids.midi_MidiEvent,
                            midi_event.size,
                            midi_event.data);
        }
    }

    midiEvents.reset();
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



bool 
lv2_adapter::process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) 
{ 
    return false; 
}


bool 
lv2_adapter::process_stereo(float **pin, float **pout, int numsamples, int const mode) 
{
    
    if (halting || mode == zzub::process_mode_no_io)
        return false;

    if(info->flags & zzub_plugin_flag_is_instrument) 
    {
        midi_track_manager.process(numsamples);
        
        if(midiEvents.count() > 0)
            send_midi_events();        
    }
    
    samp_count += numsamples;

    for(event_buf_port* eventPort: eventPorts)
        if(eventPort->flow == PortFlow::Output)
            lv2_evbuf_reset(eventPort->get_lv2_evbuf(), false);

    if(samp_count - last_update > update_every) 
    {
        ui_event_import();
        ui_event_dispatch();
        last_update = samp_count;
    }


    switch(audioInPorts.size()) {
    case 0:    // No audio inputs
        break;

    case 1: {
        float *sample = audioInPorts[0]->get_buffer();
        for (int i = 0; i < numsamples; i++)
            *sample++ = (pin[0][i] + pin[1][i]) * 0.5f;
        break;
    }

    case 2:
    default:   // FIXME if it's greater than 2, should mix some of them.
        memcpy(audioInPorts[0]->get_buffer(), pin[0], sizeof(float) * numsamples);
        memcpy(audioInPorts[1]->get_buffer(), pin[1], sizeof(float) * numsamples);
        break;
    }

    lilv_instance_run(lilvInstance, numsamples);

    /* Process any worker replies. */
    if(worker.enable) 
    {
        lv2_worker_emit_responses(&worker, lilvInstance);

        /* Notify the plugin the run() cycle is finished */
        if (worker.iface && worker.iface->end_run) {
            worker.iface->end_run(lilvInstance->lv2_handle);
        }
    }


    for(event_buf_port* port: midiPorts)
        if(port->flow == PortFlow::Input)
            lv2_evbuf_reset(port->get_lv2_evbuf(), true);


    switch(audioOutPorts.size()) 
    {
    case 0:
        return true;

    case 1:
        memcpy(pout[0], audioOutPorts[0]->get_buffer(), sizeof(float) * numsamples);
        memcpy(pout[1], audioOutPorts[0]->get_buffer(), sizeof(float) * numsamples);
        return true;

    case 2:
    default:
        memcpy(pout[0], audioOutPorts[0]->get_buffer(), sizeof(float) * numsamples);
        memcpy(pout[1], audioOutPorts[1]->get_buffer(), sizeof(float) * numsamples);
        return true;
    }
}


struct lv2plugincollection : zzub::plugincollection 
{
    lv2_lilv_world *world = lv2_lilv_world::get_instance();

    virtual void initialize(zzub::pluginfactory *factory) 
    {
        const LilvPlugins* const collection = world->get_all_plugins();
        
        LILV_FOREACH(plugins, iter, collection) 
        {
            const LilvPlugin *plugin = lilv_plugins_get(collection, iter);
            lv2_zzub_info *info = new lv2_zzub_info(world, plugin);
            factory->register_info(info);
        }
    }

    virtual const       zzub::info *get_info(const char *uri, zzub::archive *data) { return 0; }
    virtual const char *get_uri() { return 0; }
    virtual void        configure(const char *key, const char *value) {}
    virtual void        destroy() { delete this; }
};


zzub::plugincollection*
zzub_get_plugincollection() 
{
    return new lv2plugincollection();
}



const char*
zzub_get_signature() 
{ 
    return ZZUB_SIGNATURE; 
}

