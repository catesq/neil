#pragma once

#include <memory>
#include <vector>


#include "graph.h"
#include "zzub/plugin.h"
#include "zzub/zzub_data.h"

#include "libzzub/cv/transporter.h"


namespace zzub {

struct event_connection;
struct audio_connection;
struct cv_connection;
struct midi_connection;


/*************************************************************************************
 *
 * connection interface
 *
 * every edge of the audio graph - defined in graph.h - is one of these connections
 *
 ************************************************************************************/


struct connection 
{
    connection_type type;
    void* connection_values;
    std::vector<const parameter*> connection_parameters;

    virtual ~connection() {};
    virtual void process_events(zzub::song& player, const connection_descriptor& conn) = 0;
    virtual bool work(zzub::song& player, const connection_descriptor& conn, uint sample_count, uint work_position) = 0;


    connection_type get_type() const { return type; }

protected:
    // don't instantiate this class directly,
    // use either audio_connection or events_connection or midi_connection or cv_connection
    connection();
};


/*************************************************************************
 *
 * audio connection
 *
 ************************************************************************/


struct audio_connection_parameter_volume : parameter 
{
    audio_connection_parameter_volume();
};


struct audio_connection_parameter_panning : parameter 
{
    audio_connection_parameter_panning();
};


struct audio_connection_values 
{
    unsigned short amp, pan;
};


struct audio_connection : connection 
{
    static audio_connection_parameter_volume para_volume;
    static audio_connection_parameter_panning para_panning;

    audio_connection_values values;
    audio_connection_values cvalues;

    float lastFramePos;

    audio_connection();
    virtual void process_events(zzub::song& player, const zzub::connection_descriptor& conn);
    virtual bool work(zzub::song& player, const connection_descriptor& conn, uint sample_count, uint work_position);
};


/*************************************************************************
 *
 * cv_connection
 *
 ************************************************************************/


struct event_connection_binding 
{
    int source_param_index;
    int target_group_index;
    int target_track_index;
    int target_param_index;

    int get_group() const { return target_group_index; }
    int get_track() const { return target_track_index; }
    int get_param() const { return target_param_index; }
    int get_source_param() const { return source_param_index; }
};


struct event_connection : connection 
{
    std::vector<event_connection_binding> bindings;

    event_connection();

    virtual void process_events(zzub::song& player, const zzub::connection_descriptor& conn);
    virtual bool work(zzub::song& player, const connection_descriptor& conn, uint sample_count, uint work_position);

    int get_binding_count() const { return bindings.size(); }

    event_connection_binding* get_binding(unsigned index)
    {
        if (index >= bindings.size())
            return nullptr;
        else
            return &bindings[index];
    }

    void remove_binding(event_connection_binding* binding)
    {
        auto it = bindings.begin();
        while (it != bindings.end()) {
            if (
                it->get_group() == binding->get_group() && it->get_track() == binding->get_track() && it->get_param() == binding->get_param() && it->get_source_param() == binding->get_source_param()
            ) {
                it = bindings.erase(it);
            } else {
                ++it;
            }
        }
    }

    int convert(int value, const zzub::parameter* oldparam, const zzub::parameter* newparam);
    // const zzub::parameter *getParam(struct metaplugin *mp, int group, int index);
};



/*************************************************************************
 *
 * cv_connection
 *
 * cv_connections are between plugins. cv_connectors are between ports
 * 
 ************************************************************************/


struct cv_connection : connection 
{
    /**
     * there is one data transporter for each targetted port
     * one or more source ports shared this transporter 
    */
    std::vector<cv_transporter*> transporters;
    std::vector<cv_connector> connectors;

    /**
     * When the source plugin has the is_controller flag set then
     * the source plugins process_controller_events method is called
     */
    bool is_from_controller;


    cv_connection(
        bool is_from_controller
    );


    void process_events(
        zzub::song& player, 
        const zzub::connection_descriptor& conn
    );


    virtual bool work(
        zzub::song& player, 
        const connection_descriptor& conn,
        uint sample_count, 
        uint work_position
    );


    void add_connector(
        const cv_connector& link, 
        zzub::metaplugin& from,
        zzub::metaplugin& to
    );


    bool remove_connector(
        const cv_connector& link
    );


    bool has_connector(
        const cv_connector& link
    );


    const cv_connector* get_connector(
        int index
    );


    int get_connector_count() const 
    { 
        return connectors.size(); 
    }


    bool has_transporter(
        const cv_connector& link
    ); 


    bool update_connector(
        const cv_connector& old_connector, 
        const cv_connector& new_connector, 
        zzub::metaplugin& from,
        zzub::metaplugin& to
    );

protected:

    cv_transporter* get_transporter(
        const cv_connector& link
    );
};


/*************************************************************************
 *
 * midi connector
 *
 ************************************************************************/


struct midi_connection : connection 
{
    int device;
    std::string device_name;

    midi_connection();
    int get_midi_device(zzub::song& player, int plugin, std::string name);
    virtual void process_events(zzub::song& player, const zzub::connection_descriptor& conn);
    virtual bool work(zzub::song& player, const connection_descriptor& conn, uint sample_count, uint work_position);
};


} // namespace zzub
