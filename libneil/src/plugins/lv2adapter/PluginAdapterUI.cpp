#include "PluginAdapter.h"
#include <lv2/atom/atom.h>

// these are the ui related function for the PLuginAdapter - the PluginAdapter file was getting unwieldy


bool
PluginAdapter::isExternalUI(const LilvUI * ui) {
    const LilvNodes* ui_classes = lilv_ui_get_classes(ui);
    std::string name = as_string(lilv_ui_get_uri(ui));

    LILV_FOREACH (nodes, it, ui_classes) {
        const LilvNode* ui_type = lilv_nodes_get(ui_classes, it);

        if (lilv_node_equals(ui_type, lilv_new_uri(cache->lilvWorld, "http://kxstudio.sf.net/ns/lv2ext/external-ui#Widget")))
            return true;
    }

    return false;
}



bool
PluginAdapter::ui_open() {
    cache->init_suil();
    cache->init_x_threads();
    if (!suil_ui_host)
        suil_ui_host = suil_host_new(write_events_from_ui, lv2_port_index, nullptr, nullptr); //&update_port, &remove_port);

//    ui_select(use_show_interface_method ? NULL : GTK3_URI, &lilv_ui_type, &lilv_ui_type_node);

    ui_select(GTK3_URI, &lilv_ui_type, &lilv_ui_type_node);

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

//    if(!use_show_interface_method) {
        gtk_ui_window = ui_open_window(&gtk_ui_root_box, &gtk_ui_parent_box);

//    }

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
//use_show_ui ? NULL : GTK3_URI,##
    if(!suil_ui_instance) {
        suil_ui_instance = suil_instance_new(suil_ui_host,
                                             this,
                                             GTK3_URI,
                                             lilv_node_as_uri(lilv_plugin_get_uri(info->lilvPlugin)),
                                             lilv_node_as_uri(lilv_ui_get_uri(lilv_ui_type)),
                                             lilv_node_as_uri(lilv_ui_type_node),
                                             bundle_path,
                                             binary_path,
                                             ui_features);

        suil_ui_handle = suil_instance_get_handle(suil_ui_instance);
    }

//    if(use_show_interface_method) {
//        idle_interface = (const LV2UI_Idle_Interface *) suil_instance_extension_data(suil_ui_instance, LV2_UI__idleInterface);
//        show_interface = (const LV2UI_Show_Interface *) suil_instance_extension_data(suil_ui_instance, LV2_UI__showInterface);
//    }

    lilv_free(binary_path);
    lilv_free(bundle_path);

    if(!suil_ui_instance)
        return false;

    printf("get ui instance\n");
//    if(!use_show_interface_method) {
    if(!suil_widget) {
        suil_widget = (GtkWidget*)suil_instance_get_widget(suil_ui_instance);

    }
        gtk_container_add(GTK_CONTAINER(gtk_ui_parent_box), suil_widget);
        gtk_widget_show_all(gtk_ui_root_box);
        gtk_widget_grab_focus(suil_widget);
//    }

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

    printf("ui all done\n");
    ui_is_open = true;
    g_object_ref(gtk_ui_window);
//    g_object_ref(suil_widget);
//    g_object_ref(gtk_ui_root_box);
//    g_object_ref(gtk_ui_parent_box);
    return true;
}



GtkWidget*
PluginAdapter::ui_open_window(GtkWidget** root_container, GtkWidget** parent_container) {
    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
//    g_signal_connect(window, "delete-event", G_CALLBACK(on_window_destroy), this);
    g_signal_connect(window, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), this);

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

    gtk_box_pack_start(GTK_BOX(vbox), alignment, TRUE, TRUE, 0);

    gtk_widget_show_all(GTK_WIDGET(alignment));

    gtk_window_present(GTK_WINDOW(window));

    return window;
}



bool
PluginAdapter::ui_destroy() {
    printf("kill suil host\n");

    gtk_widget_hide(gtk_ui_window);
//    gtk_container_remove(GTK_CONTAINER(gtk_ui_parent_box), suil_widget);
//    gtk_widget_destroy(gtk_ui_parent_box);
//    gtk_widget_destroy(gtk_ui_root_box);
//    gtk_widget_destroy(gtk_ui_window);
//    suil_instance_free(suil_ui_instance);
//    suil_host_free(suil_ui_host);
//    gtk_ui_window    = nullptr;
//    ui_is_open       = false;
//    suil_ui_host     = nullptr;
//    suil_ui_instance = nullptr;
    return true;
}




bool
PluginAdapter::is_ui_resizable() {
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



void
PluginAdapter::ui_event_import() {
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
        } else if (ev.protocol == cache->urids.atom_eventTransfer && (port->type == PortType::Event || port->type == PortType::Midi)) {
            EventBufPort* eventPort = static_cast<EventBufPort*>(port);
            LV2_Evbuf_Iterator e = lv2_evbuf_end(eventPort->eventBuf);
            const LV2_Atom* const atom = (const LV2_Atom*)body;
            lv2_evbuf_write(&e, samp_count, 0, atom->type, atom->size, (const uint8_t*)LV2_ATOM_BODY_CONST(atom));
        } else {
            fprintf(stderr, "error: Unknown control change protocol %u\n", ev.protocol);
        }
    }
    zix_ring_reset(ui_events);
}


void
PluginAdapter::ui_event_dispatch() {

    zix_ring_reset(plugin_events);
}

void
PluginAdapter::update_all_from_ui() {
    printf("PluginAdapter::update_all_from_ui\n");

    // const char *dir = world->hostParams.tempDir;

    //  LilvState* const state = lilv_state_new_from_instance(
    //             info->lilvPlugin, pluginInstance, &world->map,
    //             dir, dir, dir, dir,
    //             get_port_value, this, LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE, nullptr);


    // printf("\n");
}



const bool
PluginAdapter::ui_select(const char *native_ui_type,
                         const LilvUI** ui_type_ui,
                         const LilvNode** ui_type_node) {
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



void
write_events_from_ui(void* const adapter_handle,
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


uint32_t
lv2_port_index(void* const adapter_handle,
               const char* symbol) {
    PluginAdapter* const adapter = (PluginAdapter *) adapter_handle;

    for(auto& port: adapter->info->ports)
        if(strncmp(port->symbol.c_str(), symbol, port->symbol.size()) == 0)
            return port->index;

    return LV2UI_INVALID_PORT_INDEX;
}


const void*
get_port_value(const char* port_symbol,
               void*       user_data,
               uint32_t*   size,
               uint32_t*   type) {
//    printf("PluginAdapter::get_port_value\n");

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


void
set_port_value(const char* port_symbol,
               void*       user_data,
               const void* value,
               uint32_t    size,
               uint32_t    type)
{
    // if(verbose)
//    printf("SET PORT VALUE %s\n", port_symbol);
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

//    if (adapter->suil_ui_instance) {
//        // Update UI
//        char buf[sizeof(ControlChange) + sizeof(fvalue)];
//        ControlChange* ev = (ControlChange*)buf;
//        ev->index    = port->index;
//        ev->protocol = 0;
//        ev->size     = sizeof(fvalue);
//        *(float*)ev->body = fvalue;
//        zix_ring_write(adapter->plugin_events, buf, sizeof(buf));
//    }
}
