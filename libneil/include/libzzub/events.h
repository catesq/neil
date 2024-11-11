#pragma once

#include "libzzub/song.h"
#include "libzzub/player.h"
#include "zzub/plugin.h"
#include "zzub/consts.h"
#include <map>
#include <vector>






namespace zzub {

inline bool is_plugin_event(zzub::event_type type)  {
    switch(type) {
        case zzub::event_type_new_plugin:
        case zzub::event_type_delete_plugin:
        case zzub::event_type_edit_pattern:
        case zzub::event_type_connect:
        case zzub::event_type_disconnect:
        case zzub::event_type_plugin_changed:
        case zzub::event_type_parameter_changed:
        case zzub::event_type_set_tracks:
        case zzub::event_type_new_pattern:
        case zzub::event_type_delete_pattern:
        case zzub::event_type_pattern_changed:
        case zzub::event_type_pattern_insert_rows:
        case zzub::event_type_pattern_remove_rows:
        case zzub::event_type_pre_delete_pattern:
        case zzub::event_type_pre_delete_plugin:
        case zzub::event_type_pre_disconnect:
        case zzub::event_type_pre_connect:
        case zzub::event_type_post_connect:
        case zzub::event_type_pre_set_tracks:
        case zzub::event_type_post_set_tracks:
            return true;

        // some events have a plugin in the event_data struct but the id for the plugin isn't sent to the event op in operations.cpp
        // case zzub::event_type_set_sequence_event:
        // case zzub::event_type_set_sequence_tracks:
        // case zzub_event_type_sequencer_add_track:
        // case zzub_event_type_sequencer_remove_track:
        // case zzub_event_type_sequencer_changed:

        default:
            return false;
    }
}

} // namespace zzub



namespace {

// returns the plugin id if the event type relates to a plugin
inline int get_event_data_plugin_id(zzub_event_data_t& data) {
    if (zzub::is_plugin_event(static_cast<zzub::event_type>(data.type)))
        return data.set_sequence_tracks.plugin->id;
    else
        return -1;
}

} // anonymous namespace



struct zzub_player_callback_all_events : zzub::event_handler {
    zzub::metaplugin_proxy* proxy = nullptr;
    zzub_flatapi_player* player;

    std::map<int, zzub::event_handler*> handlers;

    zzub_player_callback_all_events(
        zzub_flatapi_player* _player
    ) 
    {
        player = _player;
    }


    zzub_player_callback_all_events(
        zzub_flatapi_player* _player, 
        zzub::metaplugin_proxy* _proxy
    ) 
    {
        player = _player;
        proxy = _proxy;
    }

    void set_proxy(
        zzub::metaplugin_proxy* proxy
    ) 
    {
        this->proxy = proxy;
    }

    virtual bool invoke(zzub_event_data_t& data);
};



// this is the event handler for the master plugin. 
// it forwards all events to the player callback via zzub_player_callback_all_events::invoke
// and to any event handler registered with the matching (plugin id + event type), 
// -1 is the id used to listen to those events which aren't associated with a plugin and also to all events of a specified type regardless of plugin id

struct zzub_master_event_filter : zzub_player_callback_all_events {
    typedef std::pair<int, zzub::event_type> listener_id;

    std::map<listener_id, std::vector<zzub::event_handler*>> listeners;
    
    
    zzub_master_event_filter(zzub_flatapi_player* _player) 
    : zzub_player_callback_all_events(_player) 
    {
    }


    void 
    add_event_listener(
        int plugin_id, 
        zzub::event_type event_type, 
        zzub::event_handler* handler
    ) 
    {
        listeners[{plugin_id, event_type}].push_back(handler);
    }


    void 
    add_event_listener(
        zzub::event_type event_type, 
        zzub::event_handler* handler
    ) 
    {
        listeners[{-1, event_type}].push_back(handler);
    }


    void 
    remove_event_listener(
        zzub::event_handler* handler
    ) 
    {
        for(auto it = listeners.begin(); it != listeners.end();) {
            auto& handlers = it->second;

            handlers.erase(
                std::remove(handlers.begin(), handlers.end(), handler), 
                handlers.end()
            );

            if(handlers.empty())
                it = listeners.erase(it);
            else
                ++it;
        }
    }

    
    virtual bool 
    invoke(
        zzub_event_data_t& data
    ) 
    {
        dispatch(data);

        return zzub_player_callback_all_events::invoke(data);
    }


    void 
    dispatch(
        zzub_event_data_t& data
    ) 
    {
        auto plugin_id = get_event_data_plugin_id(data);

        if(plugin_id > 0 && listeners.contains({plugin_id, static_cast<zzub::event_type>(data.type)})) {
            dispatch(data, listeners[{plugin_id, static_cast<zzub::event_type>(data.type)}]);
        } else if (listeners.contains({-1, static_cast<zzub::event_type>(data.type)})) {
            dispatch(data, listeners[{-1, static_cast<zzub::event_type>(data.type)}]);
        }
    }


private:

    void 
    dispatch(
        zzub_event_data_t& data, 
        std::vector<zzub::event_handler*>& handlers
    ) 
    {
        for(auto handler: handlers)
            handler->invoke(data);
    }
};

