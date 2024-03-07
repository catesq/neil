#include "libzzub/ccm_helpers.h"
#include "libzzub/ccm_write.h"
#include "libzzub/connections.h"


namespace zzub {
/*! \struct CcmWriter
    \brief .CCM exporter
  */
xml_node CcmWriter::saveHead(xml_node &parent, zzub::song &player) {
    if (strlen(player.song_comment.c_str())) {
        // save song info
        if (arch.createFileInArchive("readme.txt")) {
            arch.write((void*)player.song_comment.c_str(),
                       (int)strlen(player.song_comment.c_str()));
            arch.closeFileInArchive();
            xml_node commentmeta = addMeta(parent, "comment");
            commentmeta.append_attribute("src") = "readme.txt";
        } else {
            std::cerr << "unable to save comment in readme.txt" << std::endl;
        }
    }
    return parent;
}

xml_node CcmWriter::addMeta(xml_node &parent, const std::string &propname) {
    xml_node item = parent.append_child(node_element);
    item.set_name("meta");
    item.attribute("name").set_name(propname.c_str());
    return item;
}

xml_node CcmWriter::saveClasses(xml_node &parent, zzub::song &player) {
    xml_node item = parent.append_child(node_element);
    item.set_name("pluginclasses");

    std::vector<const zzub::info*> distinctLoaders;
    for (int i = 0; i < player.get_plugin_count(); i++) {
        std::vector<const zzub::info*>::iterator p = find<vector<const zzub::info*>::iterator >(distinctLoaders.begin(), distinctLoaders.end(), player.get_plugin(i).info);
        if (p == distinctLoaders.end()) distinctLoaders.push_back(player.get_plugin(i).info);
    }

    for (size_t i = 0; i < distinctLoaders.size(); i++) {
        saveClass(item, *distinctLoaders[i]);
    }

    return item;
}


xml_node CcmWriter::saveParameter(xml_node &parent, const zzub::parameter &p) {

    // we take the same format as for lunar manifests

    xml_node item = parent.append_child(node_element);
    item.set_name("parameter");

    item.append_attribute("id") = id_from_ptr(&p).c_str();
    item.append_attribute("name") = p.name;

    item.append_attribute("type") = paramtype_to_string(p.type).c_str();
    item.append_attribute("minvalue") = p.value_min;
    item.append_attribute("maxvalue") = p.value_max;
    item.append_attribute("novalue") = p.value_none;
    item.append_attribute("defvalue") = p.value_default;

    if (p.flags & zzub::parameter_flag_wavetable_index) {
        item.append_attribute("waveindex") = "true";
    }
    if (p.flags & zzub::parameter_flag_state) {
        item.append_attribute("state") = "true";
    }
    if (p.flags & zzub::parameter_flag_event_on_edit) {
        item.append_attribute("editevent") = "true";
    }
    
    return item;
}

xml_node CcmWriter::saveClass(xml_node &parent, const zzub::info &info) {

    // we take the same format as for lunar manifests

    xml_node item = parent.append_child(node_element);
    item.set_name("pluginclass");
    item.append_attribute("id") = info.uri.c_str();
    if (info.flags & zzub::plugin_flag_plays_waves)
        item.append_attribute("plays_waves") = true;
    if (info.flags & zzub::plugin_flag_uses_lib_interface)
        item.append_attribute("uses_lib_interface") = true;
    if (info.flags & zzub::plugin_flag_uses_instruments)
        item.append_attribute("uses_instruments") = true;
    if (info.flags & zzub::plugin_flag_does_input_mixing)
        item.append_attribute("does_input_mixing") = true;
    if (info.flags & zzub::plugin_flag_is_root)
        item.append_attribute("is_root") = true;
    if (info.flags & zzub::plugin_flag_has_audio_input)
        item.append_attribute("has_audio_input") = true;
    if (info.flags & zzub::plugin_flag_has_audio_output)
        item.append_attribute("has_audio_output") = true;
    if (info.flags & zzub::plugin_flag_has_event_input)
        item.append_attribute("has_event_input") = true;
    if (info.flags & zzub::plugin_flag_has_event_output)
        item.append_attribute("has_event_output") = true;

    mem_archive arc;
    info.store_info(&arc);
    saveArchive(item, info.uri, arc);

    xml_node params = item.append_child(node_element);
    params.set_name("parameters");

    if (info.global_parameters.size() > 0) {
        xml_node global = params.append_child(node_element);
        global.set_name("global");

        for (size_t i = 0; i < info.global_parameters.size(); i++) {
            const zzub::parameter* p = info.global_parameters[i];
            saveParameter(global, *p).append_attribute("index") = (int)i;
        }
    }

    if (info.track_parameters.size() > 0) {
        xml_node track = params.append_child(node_element);
        track.set_name("track");

        for (size_t i = 0; i < info.track_parameters.size(); i++) {
            const zzub::parameter* p = info.track_parameters[i];
            saveParameter(track, *p).append_attribute("index") = (int)i;
        }
    }

    return item;
}



xml_node CcmWriter::saveEventBinding(xml_node &parent, zzub::event_connection_binding &binding) {
    // xml_node item = parent.append_child(node_element);
    // item.set_name("binding");
    xml_node item = parent.append_child("binding");

    item.append_attribute("source_param_index") = binding.source_param_index;
    item.append_attribute("target_group_index") = binding.target_group_index;
    item.append_attribute("target_track_index") = binding.target_track_index;
    item.append_attribute("target_param_index") = binding.target_param_index;

    return item;
}

xml_node CcmWriter::saveEventBindings(xml_node &parent, std::vector<zzub::event_connection_binding> &bindings) {
    // xml_node item = parent.append_child(node_element);
    // item.set_name("bindings");
    xml_node item = parent.append_child("bindings");

    for (size_t i = 0; i < bindings.size(); i++) {
        saveEventBinding(item, bindings[i]);
    }

    return item;
}

xml_node CcmWriter::saveCVConnector(xml_node &parent, zzub::cv_connector &link) {
    xml_node item = parent.append_child(node_element);

    item.set_name("cv_connector");
    
    item.append_attribute("source_type")        = link.source.type;
    item.append_attribute("source_value")       = link.source.value;

    item.append_attribute("target_type")        = link.target.type;
    item.append_attribute("target_value")       = link.target.value;

    item.append_attribute("data_amp")           = link.data.amp;
    item.append_attribute("data_modulate_mode") = link.data.modulate_mode;
    item.append_attribute("data_offset_before") = link.data.offset_before;
    item.append_attribute("data_offset_after")  = link.data.offset_after;

    return item;
}


xml_node CcmWriter::saveCVConnectors(xml_node &parent, std::vector<zzub::cv_connector> &connectors) {
    xml_node item = parent.append_child(node_element);
    item.set_name("cv_connectors");

    for(size_t i = 0; i < connectors.size(); i++) {
        saveCVConnector(item, connectors[i]);
    }

    return item;
}


xml_node CcmWriter::saveArchive(xml_node &parent, const std::string &pathbase, zzub::mem_archive &arc) {
    zzub::mem_archive::buffermap::iterator i;
    for (i = arc.buffers.begin(); i != arc.buffers.end(); ++i) {
        if (i->second.size()) {
            xml_node data = parent.append_child(node_element);
            data.set_name("data");
            std::string filename;
            data.append_attribute("type") = "raw";
            data.append_attribute("base") = i->first.c_str();
            if (i->first == "") {
                filename = pathbase + "/raw";
            } else {
                filename = pathbase + "/" + i->first;
            }
            arch.createFileInArchive(filename);
            arch.write(&i->second[0], i->second.size());
            arch.closeFileInArchive();
            data.append_attribute("src") = filename.c_str();
        }
    }
    return xml_node();
}

xml_node CcmWriter::saveInit(xml_node &parent, zzub::song &player, int plugin) {
    xml_node item = parent.append_child(node_element);
    item.set_name("init");

    mem_archive arc;
    metaplugin& m = *player.plugins[plugin];
    m.plugin->save(&arc);
    saveArchive(item, id_from_ptr((const void*)((long) plugin)), arc);

    if (m.info->global_parameters.size()) {
        xml_node global = item.append_child(node_element);
        global.set_name("global");

        for (size_t i = 0; i != m.info->global_parameters.size(); ++i) {
            xml_node n = global.append_child(node_element);
            n.set_name("n");
            n.append_attribute("ref") = id_from_ptr(m.info->global_parameters[i]).c_str();
            n.append_attribute("v") = player.plugin_get_parameter(plugin, 1, 0, i);
        }
    }

    if (m.tracks > 0) {
        xml_node tracks = item.append_child(node_element);
        tracks.set_name("tracks");

        for (int t = 0; t != m.tracks; ++t) {
            xml_node track = tracks.append_child(node_element);
            track.set_name("track");
            track.append_attribute("index") = t;

            for (size_t i = 0; i != m.info->track_parameters.size(); ++i) {
                xml_node n = track.append_child(node_element);
                n.set_name("n");
                n.append_attribute("ref") = id_from_ptr(m.info->track_parameters[i]).c_str();
                n.append_attribute("v") = player.plugin_get_parameter(plugin, 2, t, i);
            }
        }
    }


    if (m.plugin->attributes && m.info->attributes.size() > 0)
        saveAttributes(item, player, plugin);

    return item;
}

xml_node CcmWriter::saveAttributes(xml_node &parent, zzub::song &player, int plugin) {
    xml_node item = parent.append_child(node_element);
    item.set_name("attributes");

    metaplugin& m = *player.plugins[plugin];

    // save attributes
    for (size_t i = 0; i < m.info->attributes.size(); i++) {
        const attribute& attr = *m.info->attributes[i];
        xml_node n = item.append_child(node_element);
        n.set_name("n");
        n.append_attribute("name") = attr.name;
        n.append_attribute("v") = m.plugin->attributes[i];
    }

    return item;
}

xml_node CcmWriter::savePatternTrack(xml_node &parent, const std::string &colname, double fac, zzub::song &player, int plugin, zzub::pattern &p, int group, int track) {
    xml_node item = parent.append_child(node_element);
    item.set_name(colname.c_str());
    if (group == 2 || group == 0)
        item.append_attribute("index") = track;

    //zzub::patterntrack& t = *p.getPatternTrack(group, track);
    for (int i = 0; i < p.rows; ++i) {
        for (size_t j = 0; j < p.groups[group][track].size(); ++j) {
            const zzub::parameter *param = player.plugin_get_parameter_info(plugin, group, track, j);
            assert(param != 0);

            int value = p.groups[group][track][j][i];
            if (value != param->value_none) {
                xml_node e = item.append_child(node_element);
                e.set_name("e");
                e.append_attribute("t") = fac * double(i);
                if (group == 0) {

                    // It's not clear how amp and pan values should be saved. One possibility
                    // is to save them as "amp" and "pan" attributes under the node corresponding to
                    // to the connection itself. That's what we do here. It might be nicer to
                    // store them as ordinary "v" attributes under the node corresponding to the param:
                    // ie maybe it should be like this instead, once for amp and once for pan:
                    // e.append_attribute("ref") = id_from_ptr(param).c_str();

                    // Values are converted to double for storage and converted back during load,
                    // to avoid exposing an implementation detail. Although every other parameter
                    // is stored as an implementation-specific int value. Hmm.
                    if (j == 0) {
                        e.append_attribute("amp") = amp_to_double(value);
                    } else if (j == 1) {
                        e.append_attribute("pan") = pan_to_double(value);
                    } else {
                        assert(0);
                    }
                    // Order is important here? "ref" has to be appended *after* "amp" or "pan"
                    // I don't know why.
                    e.append_attribute("ref") = id_from_ptr(player.plugin_get_input_connection(plugin, track)).c_str();

                } else {
                    e.append_attribute("ref") = id_from_ptr(param).c_str();
                    e.append_attribute("v") = value;
                }
            }
        }
    }

    return item;
}

xml_node CcmWriter::savePattern(xml_node &parent, zzub::song &player, int plugin, zzub::pattern &p) {
    // we're going to save pattern data as events, since storing data
    // in a table is implementation detail. also, empty patterns will
    // take less space.

    // event positions and track length is stored in beats, not rows. this makes it
    // easier to change the tpb count manually, and have the loader adapt all
    // patterns on loading.

    double tpbfac = 1.0/double(player.plugin_get_parameter(0, 1, 0, 2));

    xml_node item = parent.append_child(node_element);
    item.set_name("events");
    item.append_attribute("id") = id_from_ptr(&p).c_str();
    item.append_attribute("name") = p.name.c_str();
    item.append_attribute("length") = double(p.rows) * tpbfac;

    // save connection columns
    for (size_t j = 0; j < p.groups[0].size(); ++j) {
        savePatternTrack(item, "c", tpbfac, player, plugin, p, 0, j);
    }

    // save globals
    savePatternTrack(item, "g", tpbfac, player, plugin, p, 1, 0);

    // save tracks
    for (size_t j = 0; j < p.groups[2].size(); j++) {
        savePatternTrack(item, "t", tpbfac, player, plugin, p, 2, j);
    }

    return item;
}

xml_node CcmWriter::savePatterns(xml_node &parent, zzub::song &player, int plugin) {
    xml_node item = parent.append_child(node_element);
    item.set_name("eventtracks");

    for (size_t i = 0; i != player.plugins[plugin]->patterns.size(); i++) {
        savePattern(item, player, plugin, *player.plugins[plugin]->patterns[i]);
    }

    return item;
}

xml_node CcmWriter::saveSequence(xml_node &parent, double fac, zzub::song &player, int track) {
    xml_node item = parent.append_child(node_element);
    item.set_name("sequence");
    item.append_attribute("index") = track;

    int plugin_id = player.sequencer_tracks[track].plugin_id;
    for (size_t i = 0; i < player.sequencer_tracks[track].events.size(); ++i) {
        sequence_event &ev = player.sequencer_tracks[track].events[i];

        xml_node e = item.append_child(node_element);
        e.set_name("e");
        e.append_attribute("t") = fac * double(ev.time);
        if (ev.pattern_event.value == sequencer_event_type_mute) {
            e.append_attribute("mute") = true;
        } else if (ev.pattern_event.value == sequencer_event_type_break) {
            e.append_attribute("break") = true;
        } else if (ev.pattern_event.value == sequencer_event_type_thru) {
            e.append_attribute("thru") = true;
        } else if (ev.pattern_event.value >= 0x10) {
            zzub::pattern& p = *player.plugins[plugin_id]->patterns[ev.pattern_event.value - 0x10];
            e.append_attribute("ref") = id_from_ptr(&p).c_str();
        } else {
            assert(0);
        }
    }

    return item;
}

xml_node CcmWriter::saveSequences(xml_node &parent, zzub::song &player, int plugin) {

    // event positions and track length is stored in beats, not rows. this makes it
    // easier to change the tpb count manually, and have the loader adapt all
    // sequences on loading.
    double tpbfac = 1.0/double(player.plugin_get_parameter(0, 1, 0, 2));

    xml_node item = parent.append_child(node_element);
    item.set_name("sequences");

    for (unsigned int i = 0; i != player.sequencer_tracks.size(); i++) {
        if (player.sequencer_tracks[i].plugin_id == plugin) {
            saveSequence(item, tpbfac, player, i);
        }
    }

    return item;
}

xml_node CcmWriter::saveMidiMappings(xml_node &parent, zzub::song &player, int plugin) {
    xml_node item = parent.append_child(node_element);
    item.set_name("midi");

    for (size_t j = 0; j < player.midi_mappings.size(); j++) {
        midimapping& mm = player.midi_mappings[j];
        if (mm.plugin_id == plugin) {
            xml_node bind = item.append_child(node_element);
            bind.set_name("bind");

            const zzub::parameter *param = player.plugin_get_parameter_info(plugin, mm.group, mm.track, mm.column);
            assert(param);

            if (mm.group == 0) {
                bind.append_attribute("ref") = id_from_ptr(player.plugin_get_input_connection(plugin, mm.track)).c_str();
                if (mm.column == 0) {
                    bind.append_attribute("target") = "amp";
                } else if (mm.column == 1) {
                    bind.append_attribute("target") = "pan";
                } else {
                    assert(0);
                }
                bind.append_attribute("track") = mm.track;
            } else if (mm.group == 1) {
                bind.append_attribute("ref") = id_from_ptr(param).c_str();
                bind.append_attribute("target") = "global";
            } else if (mm.group == 2) {
                bind.append_attribute("track") = mm.track;
                bind.append_attribute("ref") = id_from_ptr(param).c_str();
                bind.append_attribute("target") = "track";
            } else {
                assert(0);
            }

            bind.append_attribute("channel") = mm.channel;
            bind.append_attribute("controller") = mm.controller;
        }
    }
    return item;
}

xml_node CcmWriter::savePlugin(xml_node &parent, zzub::song &player, int plugin) {
    xml_node item = parent.append_child(node_element);
    item.set_name("plugin");
    item.append_attribute("id") = plugin;//id_from_ptr(plugin);
    item.append_attribute("name") = player.plugins[plugin]->name.c_str();
    item.append_attribute("ref") = player.plugins[plugin]->info->uri.c_str();

    // ui properties should be elsewhere
    xml_node position = item.append_child(node_element);
    position.set_name("position");
    position.append_attribute("x") = player.plugins[plugin]->x;
    position.append_attribute("y") = player.plugins[plugin]->y;

    if (player.plugin_get_input_connection_count(plugin) > 0) {
        xml_node connections = item.append_child(node_element);
        connections.set_name("connections");

        for (int i = 0; i < player.plugin_get_input_connection_count(plugin); i++) {
            xml_node item = connections.append_child(node_element);
            item.set_name("input");

            item.append_attribute("id") = id_from_ptr(player.plugin_get_input_connection(plugin, i)).c_str();
            item.append_attribute("ref") = player.plugin_get_input_connection_plugin(plugin, i);
            item.append_attribute("type") = connectiontype_to_string(player.plugin_get_input_connection_type(plugin, i)).c_str();

            switch (player.plugin_get_input_connection_type(plugin, i)) {
            case zzub::connection_type_audio:
                // we save normals, not exposing implementation details
                item.append_attribute("amplitude") = amp_to_double(player.plugin_get_parameter(plugin, 0, i, 0));
                item.append_attribute("panning") = pan_to_double(player.plugin_get_parameter(plugin, 0, i, 1));
                break;
            case zzub::connection_type_event: {
                zzub::event_connection &midi_conn = *(zzub::event_connection*)player.plugin_get_input_connection(plugin, i);
                saveEventBindings(item, midi_conn.bindings);
            } break;
            case zzub::connection_type_midi: {
                zzub::midi_connection &midi_conn = *(zzub::midi_connection*)player.plugin_get_input_connection(plugin, i);
                item.append_attribute("device") = midi_conn.device_name.c_str();
            } break;
            case zzub::connection_type_cv: {
                zzub::cv_connection &cv_conn = *(zzub::cv_connection*)player.plugin_get_input_connection(plugin, i);
                saveCVConnectors(item, cv_conn.connectors);
            } break;
            default:
                assert(0);
                break;
            }

        }
    }

    saveInit(item, player, plugin);

    saveMidiMappings(item, player, plugin);

    if (player.plugins[plugin]->patterns.size() > 0)
        savePatterns(item, player, plugin);

    saveSequences(item, player, plugin);

    return item;
}

xml_node CcmWriter::savePlugins(xml_node &parent, zzub::song &player) {
    xml_node item = parent.append_child(node_element);
    item.set_name("plugins");

    for (int i = 0; i < player.get_plugin_count(); i++) {
        savePlugin(item, player, player.get_plugin_id(i));
    }

    return item;
}

xml_node CcmWriter::saveEnvelope(xml_node &parent, zzub::envelope_entry& env) {
    xml_node item = parent.append_child(node_element);
    item.set_name("envelope");

    xml_node adsr = item.append_child(node_element);
    adsr.set_name("adsr");
    adsr.append_attribute("attack") = double(env.attack) / 65535.0;
    adsr.append_attribute("decay") = double(env.decay) / 65535.0;
    adsr.append_attribute("sustain") = double(env.sustain) / 65535.0;
    adsr.append_attribute("release") = double(env.release) / 65535.0;
    adsr.append_attribute("precision") = double(env.subDivide) / 127.0;
    // no adsr flags yet?
    // please never add flags directly, instead set boolean values.

    xml_node points = item.append_child(node_element);
    points.set_name("points");
    for (size_t i = 0; i != env.points.size(); ++i) {
        envelope_point &pt = env.points[i];
        xml_node point = points.append_child(node_element);
        point.set_name("e");
        point.append_attribute("t") = double(pt.x) / 65535.0;
        point.append_attribute("v") = double(pt.y) / 65535.0;
        if (pt.flags & zzub::envelope_flag_sustain) {
            point.append_attribute("sustain") = true;
        }
        if (pt.flags & zzub::envelope_flag_loop) {
            point.append_attribute("loop") = true;
        }
    }

    return item;
}

xml_node CcmWriter::saveEnvelopes(xml_node &parent, zzub::wave_info_ex &info) {
    xml_node item = parent.append_child(node_element);
    item.set_name("envelopes");

    for (size_t i=0; i != info.envelopes.size(); ++i) {
        if (!info.envelopes[i].disabled) {
            xml_node envnode = saveEnvelope(item, info.envelopes[i]);
            envnode.append_attribute("index") = (int)i;
        }
    }

    return item;
}


xml_node CcmWriter::saveWave(xml_node &parent, zzub::wave_info_ex &info) {
    xml_node item = parent.append_child(node_element);
    item.set_name("instrument");

    item.append_attribute("id") = id_from_ptr(&info).c_str();
    item.append_attribute("name") = info.name.c_str();
    item.append_attribute("volume") = (double)info.volume;

    saveEnvelopes(item, info);

    xml_node waves = item.append_child(node_element);
    waves.set_name("waves");

    for (int j = 0; j < info.get_levels(); j++) {
        if (info.get_sample_count(j)) {
            wave_level_ex &level = info.levels[j];
            xml_node levelnode = waves.append_child(node_element);
            levelnode.set_name("wave");
            levelnode.append_attribute("id") = id_from_ptr(&level).c_str();
            levelnode.append_attribute("index") = j;
            levelnode.append_attribute("filename") = info.fileName.c_str();

            // store root notes as midinote values
            levelnode.append_attribute("rootnote") = buzz_to_midi_note(level.root_note);

            levelnode.append_attribute("loopstart") = level.loop_start;
            levelnode.append_attribute("loopend") = level.loop_end;

            // most flags are related to levels, not to the instrument
            // this is clearly wrong and has to be fixed once we support
            // more than one wave level in the application.
            if (info.flags & zzub::wave_flag_loop) {
                levelnode.append_attribute("loop") = true;
            }
            if (info.flags & zzub::wave_flag_pingpong) {
                levelnode.append_attribute("pingpong") = true;
            }
            if (info.flags & zzub::wave_flag_envelope) {
                levelnode.append_attribute("envelope") = true;
            }

            xml_node slices = levelnode.append_child(node_element);
            slices.set_name("slices");
            for (size_t k = 0; k < level.slices.size(); k++) {
                xml_node slice = slices.append_child(node_element);
                slice.set_name("slice");
                slice.append_attribute("value") = level.slices[k];
            }

            // flac can't store anything else than default samplerates,
            // and most possibly the same goes for ogg vorbis as well
            // so save the samplerate here instead.
            levelnode.append_attribute("samplerate") = (int)info.get_samples_per_sec(j);

            // flac is going to be the standard format. maybe we're
            // going to implement ogg vorbis once the need arises.
            std::string wavename = id_from_ptr(&level) + ".flac";
            levelnode.append_attribute("src") = wavename.c_str();

            arch.createFileInArchive(wavename);
            // TODO: floats should be saved in a different format
            encodeFLAC(&arch, info, j);
            arch.closeFileInArchive();
        }

    }

    return item;
}

xml_node CcmWriter::saveWaves(xml_node &parent, zzub::song &player) {
    xml_node item = parent.append_child(node_element);
    item.set_name("instruments");

    for (size_t i = 0; i < player.wavetable.waves.size(); i++) {

        // NOTE: getting reference, ~wave_info_ex frees the sample data!!1
        wave_info_ex& info = *player.wavetable.waves[i];

        if (info.get_levels()) {
            xml_node instr = saveWave(item, info);
            instr.append_attribute("index") = (int)i;
        }
    }

    return item;
}

bool CcmWriter::save(std::string fileName, zzub::player* player) {

    if (!arch.create(fileName)) return false;

    const char* loc = setlocale(LC_NUMERIC, "C");

    pugi::xml_document xml;

    xml_node xmldesc = xml.append_child(node_pi);
    xmldesc.set_name("xml");
    xmldesc.set_value("version=\"1.0\" encoding=\"utf8\"");
    //	xmldesc.append_attribute("version") = "1.0";
    //	xmldesc.append_attribute("encoding") = "utf-8";
    xml_node xmix = xml.append_child(node_element);
    xmix.set_name("xmix");
    xmix.append_attribute("xmlns:xmix") = "http://www.zzub.org/ccm/xmix";

    // save meta information
    saveHead(xmix, player->front);

    // save main sequencer settings
    {
        double tpbfac = 1.0/double(player->front.plugin_get_parameter(0, 1, 0, 2));

        //sequencer &seq = player->song_sequencer;
        xml_node item = xmix.append_child(node_element);
        item.set_name("transport");
        item.append_attribute("bpm") = double(player->front.plugin_get_parameter(0, 1, 0, 1));
        item.append_attribute("tpb") = double(player->front.plugin_get_parameter(0, 1, 0, 2));
        item.append_attribute("seqstep") = player->front.seqstep;
        item.append_attribute("loopstart") = double(player->front.song_loop_begin) * tpbfac;
        item.append_attribute("loopend") = double(player->front.song_loop_end) * tpbfac;
        item.append_attribute("start") = double(player->front.song_begin) * tpbfac;
        item.append_attribute("end") = double(player->front.song_end) * tpbfac;
    }

    // save classes
    saveClasses(xmix, player->front);

    // save plugins
    savePlugins(xmix, player->front);

    // save waves
    saveWaves(xmix, player->front);

    std::ostringstream oss("");
    xml.print(oss);
    std::string xmlstr = oss.str();
    arch.createFileInArchive("song.xmix");
    arch.write((void*)xmlstr.c_str(), xmlstr.length());
    arch.closeFileInArchive();

    arch.close();

    setlocale(LC_NUMERIC, loc);

    return true;
}

bool CcmWriter::saveSelected(std::string filename, zzub::player* player, const int* plugins, unsigned int size) {
    printf("save selected plugins...\n");
    return false;
}


}