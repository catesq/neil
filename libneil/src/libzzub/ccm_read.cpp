
#include "libzzub/ccm_read.h"
#include "libzzub/ccm_helpers.h"
#include "libzzub/pluginloader.h"

namespace zzub {

/*! \struct CcmReader
    \brief .CCM importer
  */
void CcmReader::registerNodeById(xml_node &item) {
    if (!item.attribute("id").empty()) {
        assert(getNodeById(item.attribute("id").value()).empty()); // make sure no other node has the same id
        nodes.insert(std::pair<std::string,xml_node>(item.attribute("id").value(), item));
    }
}

xml_node CcmReader::getNodeById(const std::string &id) {
    idnodemap::iterator i = nodes.find(id);
    if (i != nodes.end())
        return i->second;
    return xml_node(); // return empty node
}

bool CcmReader::for_each(xml_node& node) {
    registerNodeById(node);
    return true;
}

bool CcmReader::loadClasses(xml_node &classes, zzub::player &player) {
    for (xml_node::iterator i = classes.begin(); i != classes.end(); ++i) {
        if (!strcmp(i->name(), "pluginclass")) {
            std::string uri = i->attribute("id").value();

            const zzub::info* loader = player.plugin_get_info(uri);
            if (!loader && uri != "@zzub.org/master") { // no loader for this
                mem_archive arc;
                // do we have some data saved?
                for (xml_node::iterator data = i->begin(); data != i->end(); ++data) {
                    if (!strcmp(data->name(), "data") && (!data->attribute("src").empty())) {
                        std::cout << "ccm: storing data for " << data->attribute("src").value() << std::endl;
                        // store data in archive
                        compressed_file_info cfi;
                        if (arch.openFileInArchive(data->attribute("src").value(), &cfi)) {
                            std::vector<char> &b = arc.get_buffer(data->attribute("base").value());
                            b.resize(cfi.uncompressed_size);
                            arch.read(&b[0], cfi.uncompressed_size);
                            arch.closeFileInArchve();
                        }
                    }
                }
                bool found = false;
                if (arc.buffers.size()) {
                    std::cout << "ccm: searching for loader for " << uri << std::endl;
                    std::vector<pluginlib*>::iterator lib;
                    for (lib = player.plugin_libraries.begin(); lib != player.plugin_libraries.end(); ++lib) {
                        if ((*lib)->collection) {
                            const zzub::info *_info = (*lib)->collection->get_info(uri.c_str(), &arc);
                            if (_info) { // library could read archive
                                (*lib)->register_info(_info); // register the new info
                                found = true;
                            }
                        }
                    }
                    if (!player.plugin_libraries.size()) {
                        std::cerr << "ccm: warning: no plugin libraries available." << std::endl;
                    }
                }
                if (!found) {
                    std::cout << "ccm: couldn't find loader for " << uri << std::endl;
                }
            }
            xml_node parameters = i->child("parameters");
            if (!parameters.empty()) {
                xml_node global = parameters.child("global");
                if (!global.empty()) {
                }
                xml_node track = parameters.child("track");
                if (!track.empty()) {
                }
            }
        }
    }
    return true;
}





bool CcmReader::loadPlugins(xml_node plugins, zzub::player &player) {

    operation_copy_flags flags;
    flags.copy_plugins = true;
    flags.copy_graph = true;
    flags.copy_wavetable = true;
    player.merge_backbuffer_flags(flags);

    std::map<std::string, int> id2plugin;

    std::vector<ccache> conns;

    for (xml_node::iterator i = plugins.begin(); i != plugins.end(); ++i) {
        if (!strcmp(i->name(), "plugin")) {
            mem_archive arc;
            xml_node position;
            xml_node init;
            xml_node attribs;
            xml_node global;
            xml_node tracks;
            xml_node connections;
            xml_node sequences;
            xml_node eventtracks;
            xml_node midi;

            // enumerate relevant nodes
            for (xml_node::iterator j = i->begin(); j != i->end(); ++j) {
                if (!strcmp(j->name(), "init")) {
                    init = *j; // store for later

                    // enumerate init section
                    for (xml_node::iterator k = j->begin(); k != j->end(); ++k) {
                        if ((!strcmp(k->name(), "data")) && (!k->attribute("src").empty())) {
                            // store data for later
                            compressed_file_info cfi;
                            if (arch.openFileInArchive(k->attribute("src").value(), &cfi)) {
                                std::vector<char> &b = arc.get_buffer(k->attribute("base").value());
                                b.resize(cfi.uncompressed_size);
                                arch.read(&b[0], cfi.uncompressed_size);
                                arch.closeFileInArchve();
                            }
                        } else if (!strcmp(k->name(), "attributes")) {
                            attribs = *k; // store for later
                        } else if (!strcmp(k->name(), "global")) {
                            global = *k; // store for later
                        } else if (!strcmp(k->name(), "tracks")) {
                            tracks = *k; // store for later
                        }
                    }
                } else if (!strcmp(j->name(), "position")) {
                    position = *j; // store for later
                } else if (!strcmp(j->name(), "connections")) {
                    connections = *j; // store for later
                } else if (!strcmp(j->name(), "sequences")) {
                    sequences = *j; // store for later
                } else if (!strcmp(j->name(), "eventtracks")) {
                    eventtracks = *j; // store for later
                } else if (!strcmp(j->name(), "midi")) {
                    midi = *j; // store for later
                }
            }

            const zzub::info *loader = player.plugin_get_info(i->attribute("ref").value());

            int plugin_id;
            if (!strcmp(i->attribute("ref").value(), "@zzub.org/master")) {
                plugin_id = 0;//player.master;
            } else if (loader) {
                std::vector<char> &b = arc.get_buffer("");

                plugin_id = player.create_plugin(b, i->attribute("name").value(), loader, 0);

            } else {
                std::cerr << "ccm: unable to find loader for uri '" << i->attribute("ref").value() << std::endl;
                plugin_id = -1;
            }

            if (plugin_id != -1 && player.back.plugins[plugin_id] != 0) {
                metaplugin& m = *player.back.plugins[plugin_id];

                id2plugin.insert(std::pair<std::string, int>(i->attribute("id").value(), plugin_id));

                if (!position.empty()) {
                    m.x = position.attribute("x").as_float();
                    m.y = position.attribute("y").as_float();
                }

                if (!attribs.empty()) {
                    for (xml_node::iterator a = attribs.begin(); a != attribs.end(); ++a) {
                        for (size_t pa = 0; pa != m.info->attributes.size(); ++pa) {
                            if (!strcmp(m.info->attributes[pa]->name, a->attribute("name").value())) {
                                m.plugin->attributes[pa] = a->attribute("v").as_int();
                            }
                        }
                    }
                }

                m.plugin->attributes_changed();

                // if (!tracks.empty()) {
                //     vector<xml_node> tracknodes;
                //     tracks.all_elements_by_name("track", std::back_inserter(tracknodes));
                //     int trackcount = (int)tracknodes.size();
                //     for (unsigned int a = 0; a != tracknodes.size(); ++a) {
                //         // if we find any track with a higher index, extend the size
                //         trackcount = std::max(trackcount, tracknodes[a].attribute("index").as_int());
                //     }

                //     player.plugin_set_track_count(plugin_id, trackcount);
                // }

                if (!tracks.empty()) {
                    auto track_nodeset = tracks.select_nodes("track");
                    int trackcount = (int)track_nodeset.size();
                    for (auto track: track_nodeset) {
                        // if we find any track with a higher index, extend the size
                        trackcount = std::max(trackcount, track.node().attribute("index").as_int());
                    }

                    player.plugin_set_track_count(plugin_id, trackcount);
                }


                // plugin default parameter values are read after connections are made
                ccache cc;
                cc.target = plugin_id;
                cc.connections = connections;
                cc.global = global;
                cc.tracks = tracks;
                cc.sequences = sequences;
                cc.eventtracks = eventtracks;
                cc.midi = midi;
                conns.push_back(cc);

            }
        }
    }

    // make connections
    for (std::vector<ccache>::iterator c = conns.begin(); c != conns.end(); ++c) {

        if (!c->connections.empty()) {
            for (xml_node::iterator i = c->connections.begin(); i != c->connections.end(); ++i) {
                if (!strcmp(i->name(), "input")) {
                    std::map<std::string, int>::iterator iplug = id2plugin.find(i->attribute("ref").value());
                    if (iplug != id2plugin.end()) {
                        std::string conntype = "audio";
                        if (!i->attribute("type").empty()) {
                            conntype = i->attribute("type").value();
                        }
                        if (conntype == "audio") {
                            if (player.plugin_add_input(c->target, iplug->second, connection_type_audio)) {
                                int amp = double_to_amp((double)i->attribute("amplitude").as_double());
                                int pan = double_to_pan((double)i->attribute("panning").as_double());
                                int track = player.back.plugin_get_input_connection_count(c->target) - 1;

                                player.plugin_set_parameter(c->target, 0, track, 0, amp, false, false, false);
                                player.plugin_set_parameter(c->target, 0, track, 1, pan, false, false, false);
                            } else
                                assert(false);
                        } else if (conntype == "event") {
                            // restore controller associations
                            player.plugin_add_input(c->target, iplug->second, connection_type_event);

                            for (xml_node::iterator j = i->begin(); j != i->end(); ++j) {
                                if (!strcmp(j->name(), "bindings")) {
                                    for (xml_node::iterator k = j->begin(); k != j->end(); ++k) {
                                        if (!strcmp(k->name(), "binding")) {
                                            //event_connection_binding binding;
                                            long source_param_index = long(k->attribute("source_param_index").as_int());
                                            long target_group_index = long(k->attribute("target_group_index").as_int());
                                            long target_track_index = long(k->attribute("target_track_index").as_int());
                                            long target_param_index = long(k->attribute("target_param_index").as_int());

                                            player.plugin_add_event_connection_binding(c->target, iplug->second,
                                                                                       source_param_index, target_group_index, target_track_index, target_param_index);
                                        }
                                    }
                                }
                            }
                        } else if (conntype == "midi") {
                            assert(false);
                            player.plugin_add_input(c->target, iplug->second, connection_type_midi);
                            player.plugin_set_midi_connection_device(c->target, iplug->second, i->attribute("device").value());
                        } else {
                            assert(0);
                        }
                    } else {
                        std::cerr << "ccm: no input " << i->attribute("ref").value() << " for connection " << i->attribute("id").value() << std::endl;
                    }

                }
            }
        }

        // now that connections are set up, we can load the plugin default state values
        if (!c->global.empty()) {
            for (xml_node::iterator i = c->global.begin(); i != c->global.end(); ++i) {
                if (!strcmp(i->name(), "n")) {
                    xml_node paraminfo = getNodeById(i->attribute("ref").value());
                    assert(!paraminfo.empty()); // not being able to deduce the index is fatal
                    if ((bool)paraminfo.attribute("state") == true) {

                        // test if the parameter names correspond with index position
                        int plugin = c->target;
                        unsigned long index = long(paraminfo.attribute("index").as_int());
                        const parameter* param = player.back.plugin_get_parameter_info(plugin, 1, 0, index);
                        std::string name = paraminfo.attribute("name").value();
                        const zzub::info* info = player.back.plugins[plugin]->info;

                        if (index < info->global_parameters.size() && param->name == name) {
                            player.plugin_set_parameter(plugin, 1, 0, index, long(i->attribute("v").as_int()), false, false, false);
                        } else {
                            // else search for a parameter name that matches
                            for (size_t pg = 0; pg != info->global_parameters.size(); ++pg) {
                                const parameter* pgp = player.back.plugin_get_parameter_info(plugin, 1, 0, pg);
                                if (pgp->name == name) {
                                    player.plugin_set_parameter(plugin, 1, 0, pg, long(i->attribute("v").as_int()), false, false, false);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
        if (!c->tracks.empty()) {
            for (xml_node::iterator i = c->tracks.begin(); i != c->tracks.end(); ++i) {
                if (!strcmp(i->name(), "track")) {
                    long t = long(i->attribute("index").as_int());
                    for (xml_node::iterator j = i->begin(); j != i->end(); ++j) {
                        if (!strcmp(j->name(), "n")) {
                            xml_node paraminfo = getNodeById(j->attribute("ref").value());
                            assert(!paraminfo.empty()); // not being able to deduce the index is fatal
                            if ((bool)paraminfo.attribute("state") == true) {
                                // test if the parameter names correspond with index position
                                int plugin = c->target;
                                unsigned long index = long(paraminfo.attribute("index").as_int());
                                const parameter* param = player.back.plugin_get_parameter_info(plugin, 2, t, index);
                                std::string name = paraminfo.attribute("name").value();
                                const zzub::info* info = player.back.plugins[plugin]->info;

                                if ((index < info->track_parameters.size()) && param->name == name) {
                                    player.plugin_set_parameter(plugin, 2, t, index, long(j->attribute("v").as_int()), false, false, false);
                                } else {
                                    // else search for a parameter name that matches
                                    for (size_t pt = 0; pt != info->track_parameters.size(); ++pt) {
                                        const parameter* ptp = player.back.plugin_get_parameter_info(plugin, 2, t, pt);
                                        if (ptp->name == name) {
                                            player.plugin_set_parameter(plugin, 2, t, pt, long(j->attribute("v").as_int()), false, false, false);
                                            break;
                                        }
                                    }
                                }

                            }
                        }
                    }
                }
            }
        }

        player.back.process_plugin_events(c->target);
    }



    // we need to create patterns in the second iteration, since the master
    // might not be neccessarily the first plugin to be initialized
    double tpbfac = double(player.back.plugin_get_parameter(0, 1, 0, 2));

    vector<int> seq_order;

    for (std::vector<ccache>::iterator c = conns.begin(); c != conns.end(); ++c) {

        if (!c->eventtracks.empty()) {
            // load and create patterns
            for (xml_node::iterator i = c->eventtracks.begin(); i != c->eventtracks.end(); ++i) {
                if (!strcmp(i->name(), "events")) {
                    int rows = int(double(i->attribute("length").as_double()) * tpbfac + 0.5);
                    int plugin = c->target;
                    pattern p;
                    player.back.create_pattern(p, plugin, rows);
                    p.name = i->attribute("name").value();
                    player.plugin_add_pattern(plugin, p);
                    int pattern_index = player.back.plugins[plugin]->patterns.size() - 1;
                    i->append_attribute("index") = pattern_index;  // set an index attribute
                    for (xml_node::iterator j = i->begin(); j != i->end(); ++j) {
                        int group = 1;
                        int track = 0;
                        if (!strcmp(j->name(), "g")) {
                            group = 1;
                        } else if (!strcmp(j->name(), "c")) {
                            group = 0;
                            track = (int)long(j->attribute("index").as_int());
                        } else if (!strcmp(j->name(), "t")) {
                            group = 2;
                            track = (int)long(j->attribute("index").as_int());
                        }


                        for (xml_node::iterator k = j->begin(); k != j->end(); ++k) {
                            if (!strcmp(k->name(), "e")) {
                                int row = int(double(k->attribute("t").as_double()) * tpbfac + 0.5);
                                assert(row < rows);
                                xml_node paraminfo = getNodeById(k->attribute("ref").value()); // not being able to deduce the index is fatal
                                assert(!paraminfo.empty());

                                long idx = long(paraminfo.attribute("index").as_int());
                                long value = 0;
                                xml_attribute v = k->attribute("v");
                                if (!v.empty()) {
                                    value = long(v.as_int());
                                } else {
                                    // maybe it's amp:
                                    v = k->attribute("amp");
                                    if (!v.empty()) {
                                        value = long(double_to_amp(v.as_double()));
                                        idx = 0;
                                    } else {
                                        // maybe it's pan:
                                        v = k->attribute("pan");
                                        if (!v.empty()) {
                                            value = long(double_to_pan(v.as_double()));
                                            idx = 1;
                                        } else {
                                            // Don't know what this node is.
                                            assert(false);
                                        }
                                    }
                                }

                                player.plugin_set_pattern_value(plugin, pattern_index, group, track, idx, row, value);
                            }
                        }
                    }
                }
            }
        }


        if (!c->sequences.empty()) {
            // load and create sequences
            for (xml_node::iterator i = c->sequences.begin(); i != c->sequences.end(); ++i) {
                if (!strcmp(i->name(), "sequence")) {

                    player.sequencer_add_track(c->target, sequence_type_pattern);

                    int seq_track = player.back.sequencer_tracks.size() - 1;
                    if (!i->attribute("index").empty())
                        seq_order.push_back(i->attribute("index").as_int());

                    for (xml_node::iterator j = i->begin(); j != i->end(); ++j) {
                        if (!strcmp(j->name(), "e")) {
                            int row = int(double(j->attribute("t").as_double()) * tpbfac + 0.5);
                            int value = -1;
                            if (bool(j->attribute("mute"))) {
                                value = sequencer_event_type_mute;
                            } else if (bool(j->attribute("break"))) {
                                value = sequencer_event_type_break;
                            } else if (bool(j->attribute("thru"))) {
                                value = sequencer_event_type_thru;
                            } else {
                                xml_node patternnode = getNodeById(j->attribute("ref").value());
                                assert(!patternnode.empty()); // shouldn't be empty
                                value = 0x10 + long(patternnode.attribute("index").as_int());
                            }
                            player.sequencer_set_event(seq_track, row, value);
                        }
                    }
                }
            }
        }


        if (!c->midi.empty()) {
            // load and set up controller bindings
            for (xml_node::iterator i = c->midi.begin(); i != c->midi.end(); ++i) {
                if (!strcmp(i->name(), "bind")) {
                    xml_node refnode = getNodeById(i->attribute("ref").value());
                    assert(!refnode.empty());

                    int channel = long(i->attribute("channel").as_int());
                    int controller = long(i->attribute("controller").as_int());

                    std::string target = i->attribute("target").value();

                    int group = 0;
                    int track = 0;
                    int index = 0;
                    if (target == "amp") {
                        group = 0;
                        index = 0;
                        track = long(i->attribute("track").as_int());
                    } else if (target == "pan") {
                        group = 0;
                        index = 1;
                        track = long(i->attribute("track").as_int());
                    } else if (target == "global") {
                        group = 1;
                        track = long(i->attribute("track").as_int());
                        index = long(refnode.attribute("index").as_int());
                    } else if (target == "track") {
                        group = 2;
                        track = long(i->attribute("track").as_int());
                        index = long(refnode.attribute("index").as_int());
                    } else {
                        assert(0);
                    }

                    player.add_midimapping(c->target, group, track, index, channel, controller);
                }
            }
        }

    }

    // fix seq_order in case of missing plugins
    for (vector<int>::iterator i = seq_order.begin(); i != seq_order.end(); ++i) {
        int j = (int)(i - seq_order.begin());
        vector<int>::iterator k;
        while ((k = find(seq_order.begin(), seq_order.end(), j)) == seq_order.end()) {
            for (k = seq_order.begin(); k != seq_order.end(); ++k) {
                if (*k >= j)
                    (*k)--;
            }
        }
    }

    // re-order the sequence tracks according to ccm
    size_t index = 0;
    for (;;) {
        if (index >= seq_order.size()) {
            break;
        }
        vector<int>::iterator tt = seq_order.begin() + index;
        if ((size_t)*tt > index) {
            int t = *tt;
            seq_order.erase(tt);
            seq_order.insert(seq_order.begin() + t, t);
            player.sequencer_move_track(index, t);
        } else
            index++;
    }

    player.flush_operations(0, 0, 0);

    return true;
}

bool CcmReader::loadInstruments(xml_node &instruments, zzub::player &player) {

    // load wave table
    // load instruments

    for (xml_node::iterator i = instruments.begin(); i != instruments.end(); ++i) {
        if (!strcmp(i->name(), "instrument")) {
            int wave_index = long(i->attribute("index").as_int());
            //wave_info_ex &info = *player.front.wavetable.waves[wave_index];
            //info.name = i->attribute("name").value();
            //info.flags = 0;
            player.wave_set_name(wave_index, i->attribute("name").value());

            xml_node waves = i->child("waves");
            if (!waves.empty()) {
                for (xml_node::iterator w = waves.begin(); w != waves.end(); ++w) {
                    if (!strcmp(w->name(), "wave")) {
                        if (arch.openFileInArchive(w->attribute("src").value())) {
                            long index = long(w->attribute("index").as_int());
                            decodeFLAC(&arch, player, wave_index, index);
                            arch.closeFileInArchve();
                            wave_info_ex &info = *player.back.wavetable.waves[wave_index];
                            wave_level_ex &level = info.levels[index];

                            int flags = info.flags;
                            if (w->attribute("loop").as_bool())
                                flags |= wave_flag_loop;
                            if (w->attribute("pingpong").as_bool())
                                flags |= wave_flag_pingpong;
                            if (w->attribute("envelope").as_bool())
                                flags |= wave_flag_envelope;

                            player.wave_set_flags(wave_index, flags);
                            player.wave_set_path(wave_index, w->attribute("filename").value());
                            player.wave_set_root_note(wave_index, index, midi_to_buzz_note(long(w->attribute("rootnote").as_int())));
                            player.wave_set_loop_begin(wave_index, index, long(w->attribute("loopstart").as_int()));
                            player.wave_set_loop_end(wave_index, index, long(w->attribute("loopend").as_int()));
                            if (!w->attribute("samplerate").empty()) {
                                player.wave_set_samples_per_second(wave_index, index, long(w->attribute("samplerate").as_int()));
                            }

                            xml_node slices = w->child("slices");
                            if (!slices.empty()) {
                                for (xml_node::iterator s = slices.begin(); s != slices.end(); ++s) {
                                    if (!strcmp(s->name(), "slice")) {
                                        level.slices.push_back(long(s->attribute("value").as_int()));
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // NOTE: must set volume after (first) allocate_level (which happens in decodeFLAC)
            player.wave_set_volume(wave_index, i->attribute("volume").as_float());

            vector<envelope_entry> envs;
            xml_node envelopes = i->child("envelopes");
            if (!envelopes.empty()) {
                for (xml_node::iterator e = envelopes.begin(); e != envelopes.end(); ++e) {
                    if (!strcmp(e->name(), "envelope")) {
                        //wave_info_ex &info = *player.back.wavetable.waves[wave_index];
                        long index = long(e->attribute("index").as_int());
                        if (envs.size() <= (size_t)index) {
                            envs.resize(index+1);
                        }
                        envelope_entry &env = envs[index];
                        env.disabled = false;
                        xml_node adsr = e->child("adsr");
                        if (!adsr.empty()) {
                            env.attack = int(double(adsr.attribute("attack").as_double()) * 65535.0 + 0.5);
                            env.decay = int(double(adsr.attribute("decay").as_double()) * 65535.0 + 0.5);
                            env.sustain = int(double(adsr.attribute("sustain").as_double()) * 65535.0 + 0.5);
                            env.release = int(double(adsr.attribute("release").as_double()) * 65535.0 + 0.5);
                            env.subDivide = int(double(adsr.attribute("precision").as_double()) * 127.0 + 0.5);
                        }
                        xml_node points = e->child("points");
                        if (!points.empty()) {
                            env.points.clear();
                            for (xml_node::iterator pt = points.begin(); pt != points.end(); ++pt) {
                                if (!strcmp(pt->name(), "e")) {
                                    envelope_point evpt;
                                    evpt.x = int(double(pt->attribute("t").as_double()) * 65535.0 + 0.5);
                                    evpt.y = int(double(pt->attribute("v").as_double()) * 65535.0 + 0.5);
                                    evpt.flags = 0;
                                    if (pt->attribute("sustain").as_bool()) {
                                        evpt.flags |= zzub::envelope_flag_sustain;
                                    }
                                    if (pt->attribute("loop").as_bool()) {
                                        evpt.flags |= zzub::envelope_flag_loop;
                                    }
                                    env.points.push_back(evpt);
                                    // TODO: sort points. points might not be written down
                                    // in correct order
                                }
                            }
                        }
                    }
                }
            }
            player.wave_set_envelopes(wave_index, envs);

        }
    }

    return true;
}

bool CcmReader::loadSequencer(xml_node &item, zzub::player &player) {
    double tpbfac = double(player.front.plugin_get_parameter(0, 1, 0, 2));

    //sequencer &seq = player.song_sequencer;
    player.front.seqstep = int(item.attribute("seqstep").as_int());
    player.front.song_loop_begin = int(double(item.attribute("loopstart").as_double()) * tpbfac + 0.5);
    player.front.song_loop_end = int(double(item.attribute("loopend").as_double()) * tpbfac + 0.5);
    player.front.song_begin = int(double(item.attribute("start").as_double()) * tpbfac + 0.5);
    player.front.song_end = int(double(item.attribute("end").as_double()) * tpbfac + 0.5);
    return true;
}


bool CcmReader::open(std::string fileName, zzub::player* player) {
    const char* loc = setlocale(LC_NUMERIC, "C");

    bool result = false;
    player->set_state(player_state_muted);

    if (arch.open(fileName)) {
        compressed_file_info cfi;
        if (arch.openFileInArchive("song.xmix", &cfi)) {
            char *xmldata = new char[cfi.uncompressed_size+1];
            xmldata[cfi.uncompressed_size] = '\0';
            arch.read(xmldata, cfi.uncompressed_size);
            arch.closeFileInArchve();
            xml_document xml;
            if (xml.load_string(xmldata)) {
                xml_node xmix = xml.child("xmix");
                if (!xmix.empty()) {
                    xmix.traverse(*this); // collect all ids

                    // load song meta information
                    for (xml_node::iterator m = xmix.begin(); m != xmix.end(); ++m) {
                        if (!strcmp(m->name(), "meta")) {
                            // if (!strcmp(m->attribute("name").value(), "comment") && !m->attribute("src").empty()) {
                            if (!m->attribute("src").empty()) {
                                if (arch.openFileInArchive(m->attribute("src").value(), &cfi)) {
                                    std::vector<char> infotext;
                                    infotext.resize(cfi.uncompressed_size + 1);
                                    infotext[cfi.uncompressed_size] = '\0';
                                    arch.read(&infotext[0], cfi.uncompressed_size);
                                    arch.closeFileInArchve();
                                    player->front.song_comment = &infotext[0];
                                } else {
                                    std::cerr << "unable to open " << m->attribute("src").value() << " for reading." << std::endl;
                                }
                            }
                        }
                    }

                    // load parameter predefs
                    xml_node classes = xmix.child("pluginclasses");
                    if (classes.empty() || loadClasses(classes, *player)) {
                        // load plugins (connections, parameters, patterns, sequences)
                        xml_node plugins = xmix.child("plugins");
                        if (plugins.empty() || loadPlugins(plugins, *player)) {
                            // load instruments (waves, envelopes)
                            xml_node instruments = xmix.child("instruments");
                            if (instruments.empty() || loadInstruments(instruments, *player)) {
                                // load song sequencer settings
                                xml_node sequencer = xmix.child("transport");
                                if (sequencer.empty() || loadSequencer(sequencer, *player)) {
                                    result = true;
                                }
                            }
                        }
                    }

                } else {
                    std::cerr << "ccm: no xmix node in song.xmix from " << fileName << std::endl;
                }
            } else {
                std::cerr << "ccm: error parsing song.xmix in " << fileName << std::endl;
            }
            delete[] xmldata;
        } else {
            std::cerr << "ccm: error opening song.xmix in " << fileName << std::endl;
        }
        arch.close();
    } else {
        std::cerr << "ccm: error opening " << fileName << std::endl;
    }

    player->set_state(player_state_stopped);
    player->flush_operations(0, 0, 0);
    player->flush_from_history();	// TODO: ccm loading doesnt support undo yet


    setlocale(LC_NUMERIC, loc);

    return result;
}


}