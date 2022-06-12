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

const char *zzub_adapter_name = "zzub lv2 adapter";


bool
PluginAdapter::isExternalUI(const LilvUI * ui) {
    printf("UI: %lu\n", (long unsigned) ui);
    const LilvNodes* ui_classes = lilv_ui_get_classes(ui);
    printf("UI classes: %lu\n", (long unsigned) ui);

    std::string name = as_string(lilv_ui_get_uri(ui));
    printf("UI name: %s", name.c_str());

    LILV_FOREACH (nodes, it, ui_classes) {
        const LilvNode* ui_type = lilv_nodes_get (ui_classes, it);

        if (lilv_node_equals(ui_type, lilv_new_uri(cache->lilvWorld, "http://kxstudio.sf.net/ns/lv2ext/external-ui#Widget"))) {
            return true;
        }
    }

    return false;
}



PluginAdapter::PluginAdapter(PluginInfo *info) : info(info), cache(info->cache) {
    if (verbose) { printf("plugin <%s>: in constructor. data size: %d\n", info->name.c_str(), info->zzubTotalDataSize); }

    if(info->zzubTotalDataSize) {
        global_values = malloc(info->zzubTotalDataSize);
        memset(global_values, 0, info->zzubTotalDataSize);
    }

    uis           = lilv_plugin_get_uis(info->lilvPlugin);
    ui_events     = zix_ring_new(EVENT_BUF_SIZE);
    plugin_events = zix_ring_new(EVENT_BUF_SIZE);

    track_values = &trak_values[0];
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
                ports.push_back(std::ref(audioInPorts.back()));
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
            eventPorts.back()->eventBuf = lv2_evbuf_new(cache->hostParams.bufSize, cache->urids.atom_Chunk, cache->urids.atom_Sequence);
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


void PluginAdapter::init(zzub::archive *arc) {
    bool use_show_ui = false;
    sample_rate = _master_info->samples_per_second;
    ui_scale = get_scale_factor();

    LV2_Options_Option options[] = {
       {
            LV2_OPTIONS_INSTANCE,
            0,
            cache->urids.param_sampleRate,
            sizeof(float),
            cache->urids.atom_Float,
            &sample_rate
       },
       {
            LV2_OPTIONS_INSTANCE, 0,
            cache->urids.minBlockLength,
            sizeof(int32_t),
            cache->urids.atom_Int,
            &cache->hostParams.minBlockLength
       },
       {
            LV2_OPTIONS_INSTANCE, 0,
            cache->urids.maxBlockLength,
            sizeof(int32_t),
            cache->urids.atom_Int,
            &cache->hostParams.blockLength
       },
       {
            LV2_OPTIONS_INSTANCE, 0,
            cache->urids.bufSeqSize,
            sizeof(int32_t),
            cache->urids.atom_Int,
            &cache->hostParams.bufSize
       },
       {
            LV2_OPTIONS_INSTANCE, 0,
            cache->urids.ui_updateRate,
            sizeof (float),
            cache->urids.atom_Float,
            &update_rate
       },
       {
            LV2_OPTIONS_INSTANCE, 0,
            cache->urids.ui_scaleFactor,
            sizeof (float),
            cache->urids.atom_Float,
            &ui_scale
       },
       {
            LV2_OPTIONS_INSTANCE, 0,
            cache->urids.ui_transientWindowId,
            sizeof (long),
            cache->urids.atom_Long,
            &transient_wid
       },
       {
            LV2_OPTIONS_INSTANCE,
            0,
            0,
            0,
            0,
            NULL
        }
    };

    features.options = malloc(sizeof(options));
    memcpy(features.options, &options[0], sizeof(options));

    features.map_feature            = { LV2_URID__map,                    &cache->map };
    features.unmap_feature          = { LV2_URID__unmap,                  &cache->unmap };
    features.bounded_buf_feature    = { LV2_BUF_SIZE__boundedBlockLength, &cache->hostParams.blockLength };
    features.make_path_feature      = { LV2_STATE__makePath,              &cache->make_path };
    features.options_feature        = { LV2_OPTIONS__options,             features.options};
    features.program_host_feature   = { LV2_PROGRAMS__Host,               &features.program_host };
    features.data_access_feature    = { LV2_DATA_ACCESS_URI,              NULL };
    features.worker_feature         = { LV2_WORKER__schedule,             &features.worker_schedule };

    features.ui_parent_feature      = { LV2_UI__parent  ,                 NULL};
    features.ui_instance_feature    = { LV2_INSTANCE_ACCESS_URI,          NULL };
    features.ui_idle_feature        = { LV2_UI__idleInterface,            NULL };
    features.ui_data_access_feature = { LV2_DATA_ACCESS_URI,              &features.ext_data };

    features.program_host.handle           = this;
    features.program_host.program_changed  = &program_changed;

    worker.plugin                          = this;
    features.worker_schedule.handle        = &worker;
    features.worker_schedule.schedule_work = &lv2_worker_schedule;

    zix_sem_init(&worker.sem, 0);
    zix_sem_init(&work_lock, 1);

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

    lilvInstance = lilv_plugin_instantiate(info->lilvPlugin, _master_info->samples_per_second, feature_list);

    metaPlugin     = _host->get_metaplugin();
    _host->set_event_handler(metaPlugin, this);

    features.ext_data.data_access = lilv_instance_get_descriptor(lilvInstance)->extension_data;
    features.ui_instance_feature.data = lilv_instance_get_handle(lilvInstance);

    worker.enable = lilv_plugin_has_extension_data(info->lilvPlugin, cache->nodes.worker_iface);
    if (worker.enable) {
        auto iface = lilv_instance_get_extension_data (lilvInstance, LV2_WORKER__interface);
        lv2_worker_init(this, &worker, (const LV2_Worker_Interface*) iface, true);
    }

    connectInstance(lilvInstance);

    lilv_instance_activate(lilvInstance);
}



bool PluginAdapter::ui_open() {
    attr_values.ui = 0;
    bool use_show_ui = false;

    suil_ui_host = suil_host_new(write_events_from_ui, lv2_port_index, nullptr, nullptr); //&update_port, &remove_port);
    ui_select(use_show_ui ? NULL : GTK3_URI, &lilv_ui_type, &lilv_ui_type_node);

    if(lilv_ui_type != NULL) {
        std::string uiname = as_string(lilv_ui_type_node);
        isExternalUI(lilv_ui_type);
    }

    auto datatypes = lilv_plugin_get_extension_data(info->lilvPlugin);
    LILV_FOREACH(nodes, n, datatypes) {
        printf("Ext data type: %s\n", lilv_node_as_uri(lilv_nodes_get(datatypes, n)));
    }

    if(!lilv_ui_type)
        return false;

    if(!use_show_ui) {
        gtk_ui_window = ui_open_window(&gtk_ui_root_box, &gtk_ui_parent_box);
    }

    const char* bundle_uri  = lilv_node_as_uri(lilv_ui_get_bundle_uri(lilv_ui_type));
    const char* binary_uri  = lilv_node_as_uri(lilv_ui_get_binary_uri(lilv_ui_type));
    char*       bundle_path = lilv_file_uri_parse(bundle_uri, NULL);
    char*       binary_path = lilv_file_uri_parse(binary_uri, NULL);

    transient_wid = _host->get_host_info()->host_ptr;

    const LV2_Feature* ui_features[] = {
        &features.map_feature,
        &features.unmap_feature,
        &features.program_host_feature,
        &features.ui_parent_feature,
        &features.ui_instance_feature,
        &features.ui_data_access_feature,
        &features.ui_idle_feature,
        &features.options_feature,
        NULL
    };

    suil_ui_instance = suil_instance_new(suil_ui_host,
                                         this,
                                         use_show_ui ? NULL : GTK3_URI,
                                         lilv_node_as_uri(lilv_plugin_get_uri(info->lilvPlugin)),
                                         lilv_node_as_uri(lilv_ui_get_uri(lilv_ui_type)),
                                         lilv_node_as_uri(lilv_ui_type_node),
                                         bundle_path,
                                         binary_path,
                                         ui_features);

    suil_ui_handle = suil_instance_get_handle(suil_ui_instance);

    if(use_show_ui) {
        idle_interface = (const LV2UI_Idle_Interface *) suil_instance_extension_data(suil_ui_instance, LV2_UI__idleInterface);
        show_interface = (const LV2UI_Show_Interface *) suil_instance_extension_data(suil_ui_instance, LV2_UI__showInterface);
    }

    lilv_free(binary_path);
    lilv_free(bundle_path);

    if(!suil_ui_instance)
        return false;

    if(!use_show_ui) {
        GtkWidget* suil_widget = (GtkWidget*)suil_instance_get_widget(suil_ui_instance);
        gtk_container_add(GTK_CONTAINER(gtk_ui_parent_box), suil_widget);
        gtk_widget_show_all(gtk_ui_root_box);
        gtk_widget_grab_focus(suil_widget);
    }

        // Set initial control port values
    for (auto& paramPort: paramPorts) {
        suil_instance_port_event(
            suil_ui_instance,
            paramPort->index,
            sizeof(float),
            0,
            &paramPort->value
        );
    }

    for (auto& controlPort: controlPorts) {
        suil_instance_port_event(
            suil_ui_instance,
            controlPort->index,
            sizeof(float),
            0,
            &controlPort->value
        );
    }


    if(features.ui_idle_feature.data) {
        printf("Idle feature for %s in invoke\n", info->name.c_str());
    }

    attr_values.ui = 1;
    printf("leave PluginAdapter::ui_open\n");
    return true;
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


bool PluginAdapter::is_ui_resizable() {
    if(!lilv_ui_type)
        return false;

    const LilvNode* s   = lilv_ui_get_uri(lilv_ui_type);
    LilvNode*       p   = lilv_new_uri(cache->lilvWorld, LV2_CORE__optionalFeature);
    LilvNode*       fs  = lilv_new_uri(cache->lilvWorld, LV2_UI__fixedSize);
    LilvNode*       nrs = lilv_new_uri(cache->lilvWorld, LV2_UI__noUserResize);

    LilvNodes* fs_matches  = lilv_world_find_nodes(cache->lilvWorld, s, p, fs);
    LilvNodes* nrs_matches = lilv_world_find_nodes(cache->lilvWorld, s, p, nrs);

    lilv_nodes_free(nrs_matches);
    lilv_nodes_free(fs_matches);
    lilv_node_free(nrs);
    lilv_node_free(fs);
    lilv_node_free(p);

    return !fs_matches && !nrs_matches;
}

GtkWidget* PluginAdapter::ui_open_window(GtkWidget** root_container, GtkWidget** parent_container) {
    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), this);

    gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(_host->get_host_info()->host_ptr));
    gtk_window_set_title(GTK_WINDOW(window), info->name.c_str());

    gtk_window_set_role(GTK_WINDOW(window), "plugin_ui");
    gtk_window_set_resizable(GTK_WINDOW(window), is_ui_resizable());

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget* alignment = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);

    *parent_container               = alignment;
    *root_container                 = vbox;
    features.ui_parent_feature.data = *parent_container;
    ui_scale                        = gtk_widget_get_scale_factor(window);
    printf("is this a window 1?\n");

    gtk_box_pack_start(GTK_BOX(vbox), alignment, TRUE, TRUE, 0);
    printf("is this a widget 1?\n");

    gtk_widget_show_all(GTK_WIDGET(alignment));

    printf("is this a window 1b?\n");

    gtk_window_present(GTK_WINDOW(window));

    return window;
}



void PluginAdapter::ui_destroy() {
    gtk_widget_destroy(gtk_ui_window);
    suil_instance_free(suil_ui_instance);
    suil_host_free(suil_ui_host);
    gtk_ui_window    = nullptr;
    suil_ui_host     = nullptr;
    suil_ui_instance = nullptr;
}



const bool PluginAdapter::ui_select(const char *native_ui_type, const LilvUI** ui_type_ui, const LilvNode** ui_type_node) {
    // native_ui_type is
    //   nullptr if LV2_UI__showInterface was a required feature for this plugin
    //   GTK3    if LV2_UI__showInterface is an optional feature or not supported



    if(native_ui_type != nullptr) {
        // Try to find an embeddable UI
        LilvNode* native_type = lilv_new_uri(cache->lilvWorld, native_ui_type);

        LILV_FOREACH (uis, u, uis) {
            const LilvUI*   ui_type   = lilv_uis_get(uis, u);
            const LilvNode* ui_node   = NULL;
            const bool      supported = lilv_ui_is_supported(ui_type, suil_ui_supported, native_type, &ui_node);

            if (supported) {
                const char *str = lilv_node_as_uri(ui_node);
                lilv_node_free(native_type);

                *ui_type_node = ui_node;
                *ui_type_ui = ui_type;
                return true;
            }
        }

        lilv_node_free(native_type);
        return false;

    } else  {
        // Try to find a UI with ui:showInterface
        LILV_FOREACH (uis, u, uis) {
            const LilvUI*   ui      = lilv_uis_get(uis, u);
            const LilvNode* ui_node = lilv_ui_get_uri(ui);

            lilv_world_load_resource(cache->lilvWorld, ui_node);

            const bool supported = lilv_world_ask(cache->lilvWorld,
                                                  ui_node,
                                                  cache->nodes.extensionData,
                                                  cache->nodes.showInterface);

            lilv_world_unload_resource(cache->lilvWorld, ui_node);

            if (supported) {
                *ui_type_node = ui_node;
                *ui_type_ui = ui;
                return true;
            }
        }
    }

    return false;
}


void program_changed(LV2_Programs_Handle handle, int32_t index) {
    // printf("program changed\n");
    printf("PluginAdapter::program_changed\n");

    PluginAdapter* adapter = (PluginAdapter*) handle;
    adapter->update_all_from_ui();
    adapter->update_from_program_change = true;
}


void PluginAdapter::destroy() {
    delete this;
}



void PluginAdapter::connectInstance(LilvInstance* pluginInstance) {
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
                get_port_value, this, LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE, nullptr);

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


void PluginAdapter::ui_event_import() {
    ControlChange ev;
    const size_t  space = zix_ring_read_space(ui_events);
    for (size_t i = 0; i < space; i += sizeof(ev) + ev.size) {
        zix_ring_read(ui_events, (char*)&ev, sizeof(ev));
        char body[ev.size];  

        if (zix_ring_read(ui_events, body, ev.size) != ev.size) {
            fprintf(stderr, "error: Error reading from UI ring buffer\n");
            break;
        }
        assert(ev.index < info->ports.size());
        Port* port = ports[ev.index];

        if (ev.protocol == 0 && port->type == PortType::Param) {
            update_port(static_cast<ParamPort*>(port), *((float*) body));
        } else if (ev.protocol == cache->urids.atom_eventTransfer && port->type == PortType::Event) {
            EventBufPort* eventPort = static_cast<EventBufPort*>(port);
            LV2_Evbuf_Iterator e = lv2_evbuf_end(eventPort->eventBuf);

            const LV2_Atom* const atom = (const LV2_Atom*)body;
            lv2_evbuf_write(&e, samp_count, 0, atom->type, atom->size,
                            (const uint8_t*)LV2_ATOM_BODY_CONST(atom));
        } else {
            fprintf(stderr, "error: Unknown control change protocol %u\n",
                    ev.protocol);
        }
    }
}

void PluginAdapter::update_port(ParamPort* port, float float_val) {
    printf("Update port: %s => %f\n", port->name.c_str(), float_val);
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

//    if(ui_instance != nullptr) {
//        if(gtk_wi:qdget_is_visible((GtkWidget*)suil_instance_get_widget(ui_instance))) {
//            printf("widget is visible\n");
//        } else {
//            printf("widget is not visible\n");
//        }
//    }

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

    for (int t = 0; t < trackCount; t++) {
        if (trak_values[t].volume != TRACKVAL_VOLUME_UNDEFINED)

            trak_states[t].volume = trak_values[t].volume;

        if (trak_values[t].note == zzub::note_value_none) {

            if(trak_values[t].volume != TRACKVAL_VOLUME_UNDEFINED && trak_states[t].note != zzub::note_value_none)
                midiEvents.aftertouch(attr_values.channel, trak_states[t].note, trak_states[t].volume);

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

        process_track_midi_events(trak_values[t].msg_1, trak_states[t].msg_1);
        process_track_midi_events(trak_values[t].msg_2, trak_states[t].msg_2);
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


void PluginAdapter::update_all_from_ui() {
    printf("PluginAdapter::update_all_from_ui\n");

    // const char *dir = world->hostParams.tempDir;

    //  LilvState* const state = lilv_state_new_from_instance(
    //             info->lilvPlugin, pluginInstance, &world->map,
    //             dir, dir, dir, dir,
    //             get_port_value, this, LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE, nullptr);


    // printf("\n");
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
    if (halting || mode == zzub::process_mode_no_io) {
        // printf("mode no io process_stereo() returning false\n");
        return false;
    }

    for(EventBufPort* midiPort: midiPorts) {
        if(midiPort->flow != PortFlow::Input) {
            continue;
        }

        lv2_evbuf_reset(midiPort->eventBuf, true);

        if(midiEvents.count() == 0) {
            continue;
        }

        LV2_Evbuf_Iterator buf_iter = lv2_evbuf_begin(midiPort->eventBuf);

        for (auto& midi_event: midiEvents.data) {
            lv2_evbuf_write(&buf_iter, 
                             midi_event.time, 0,
                             cache->urids.midi_MidiEvent,
                             midi_event.size,
                             midi_event.data);
        }

        midiEvents.reset();
    }

    samp_count += numsamples;
    if(samp_count - last_update > update_every) {
        ui_event_import();
        last_update = samp_count;
    }

    switch(audioInPorts.size()) {
    case 0:    // No audio inputs
        break;

    case 1: {
        float *sample = audioInPorts[0]->buf;
        for (int i = 0; i < numsamples; i++) {
            *sample++ = (pin[0][i] + pin[1][i]) * 0.5f;
        }
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


    for(EventBufPort* port: eventPorts) {
        if(port->flow == PortFlow::Input) {
            continue;
        }

        lv2_evbuf_reset(port->eventBuf, true);
    }


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
    SharedAdapterCache *world = SharedAdapterCache::getInstance();
   
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

    if (protocol != 0 && protocol != adapter->cache->urids.atom_eventTransfer) {
        fprintf(stderr, "UI write with unsupported protocol %u (%s)\n", protocol, unmap_uri(adapter->cache, protocol));
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

//    Port* port = adapter->info->port_by_symbol(symbol);
    for(auto& port: adapter->info->ports)
        if(strncmp(port->symbol.c_str(), symbol, port->symbol.size()) == 0)
            return port->index;

    return LV2UI_INVALID_PORT_INDEX;
}


const void* get_port_value(const char* port_symbol,
                           void*       user_data,
                           uint32_t*   size,
                           uint32_t*   type) {
    printf("PluginAdapter::get_port_value\n");

    PluginAdapter* adapter = (PluginAdapter*) user_data;
    ParamPort* port = adapter->get_param_port(port_symbol);

    if (port != nullptr) {
        *size = sizeof(float);
        *type = adapter->cache->forge.Float;
        return &port->value;
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
    if (type == adapter->cache->forge.Float) {
        fvalue = *(const float*)value;
    } else if (type == adapter->cache->forge.Double) {
        fvalue = *(const double*)value;
    } else if (type == adapter->cache->forge.Int) {
        fvalue = *(const int32_t*)value;
    } else if (type == adapter->cache->forge.Long) {
        fvalue = *(const int64_t*)value;
    } else {
        fprintf(stderr, "error: Preset `%s' value has bad type <%s>\n",
                port_symbol, adapter->cache->unmap.unmap(adapter->cache->unmap.handle, type));
        return;
    }

    port->value = fvalue;

    if (adapter->suil_ui_instance) {
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
