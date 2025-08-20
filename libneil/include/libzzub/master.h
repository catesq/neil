/*
Copyright (C) 2003-2008 Anders Ervik <calvin@countzero.no>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#include <vector>
#include <utility>
#include <string>

#include "libzzub/ports.h"

namespace zzub {

struct archive;
struct master_metaplugin;

struct master_values {
    unsigned short volume;
    unsigned short bpm;
    unsigned char tpb;
};

struct master_plugin : public port_facade_plugin, public event_handler {
    master_values* gvals;
    master_values dummy;

    // any ports connected here are used by the multi track recorder
    std::vector<zzub::port*> recorder_ports{};
    
    std::vector<std::pair<int, std::string> > midi_devices;
    int master_volume;
    int samples_per_second;
    std::vector<cv_connector> recorder_connections;

    master_plugin();

    virtual void init(zzub::archive*);
    virtual void process_events();

    virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);

    void update_tempo(int bpm, int tpb);
    void update_midi_devices();

    // event_handler method
    virtual bool invoke(zzub_event_data_t &data);
    virtual void process_midi_events(midi_message *pin, int nummessages);
    virtual void get_midi_output_names(outstream *pout);

    virtual void created();
    virtual bool connect_ports(cv_connector& connnector);
    virtual void disconnect_ports(cv_connector& connnector);
    virtual void destroy() { delete this; }
};


struct master_plugin_info : zzub::info {
    master_plugin_info();
    virtual zzub::plugin* create_plugin() const;
    virtual bool store_info(zzub::archive *) const;
};

}
