#include "libzzub/events.h"



bool 
zzub_player_callback_all_events::invoke(zzub_event_data_t& data) {
    // the master plugin checks for create/delete plugin events and maintains an
    // array of handlers who forward all events to the player callback.
    if (proxy->id == 0 && data.type == zzub_event_type_new_plugin) {
        zzub::metaplugin_proxy* new_plugin = data.new_plugin.plugin;
        zzub_player_callback_all_events *ev = new zzub_player_callback_all_events(player, new_plugin);
        handlers[new_plugin->id] = ev;
        player->front.plugins[new_plugin->id]->event_handlers.push_back(ev);
    } else if (proxy->id == 0 && data.type == zzub_event_type_pre_delete_plugin) {
        zzub::metaplugin_proxy* del_plugin = data.delete_plugin.plugin;
        std::map<int, event_handler*>::iterator i = handlers.find(del_plugin->id);
        if (i != handlers.end()) {
            delete i->second;
            handlers.erase(i);
        }
    }

    if (player->callback) {
        //int plugin = player->front.plugins[plugin_id]->descriptor;
        int res = player->callback(player, proxy, &data, player->callbackTag);
        if (!res)
            return true;
    } else {
        player->push_event(data);
    }
    return false;
}
