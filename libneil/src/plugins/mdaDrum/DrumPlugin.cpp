
#include "DrumDefines.h"
#include "DrumPlugin.h"
#include "DrumVoice.h"
#include "DrumPresets.h"
#include "DrumParameter.h"

#include <string>
#include <stdio.h>



void DrumTvals::reset() {
    drumId = 0;
    note = 0;
    volume = 255;
    stretch = 0xffff;
}

float DrumTvals::amplitude() {
    return volume == 0xff ? 1.0f : volume / 128.0f;
}

float DrumTvals::timestretch() {
    return stretch == 0xffff ? 1.0f : stretch / 256.0f;
}


DrumPlugin::DrumPlugin(const DrumSets* drumSets) : drumSets(drumSets) {
    track_values = tval;
}

DrumPlugin::~DrumPlugin() {
    delete_voices(track_voices);
    delete_voices(active_voices);
    delete_voices(inactive_voices);
}

void DrumPlugin::delete_voices(std::vector<DrumVoice*>& voices) {
    for(DrumVoice*& voice: voices) {
        if(voice !=  nullptr) {
            delete voice;
            voice = nullptr;
        }
    }
    voices.clear();    
}

void DrumPlugin::event(unsigned int data) {
    printf("event data %d\n", data);
}

void DrumPlugin::init(zzub::archive *arc) {
    if(arc)
        load(arc);
}

void DrumPlugin::save(zzub::archive *arc) {
    zzub::outstream *po = arc->get_outstream("");
    po->write(&track_count, sizeof(int));
    for(int i=0; i< track_count; i++) {
        po->write(&tstate[i].drumId, sizeof(uint16_t));
    }
}

void DrumPlugin::load(zzub::archive *arc) {
    zzub::instream *pi = arc->get_instream("");

    if(pi->size() < sizeof(int))
        return;

    int num_tracks;
    pi->read(&num_tracks, sizeof(int));

    int expected = sizeof(uint16_t) * num_tracks + sizeof(int);
    if(num_tracks <= 0 || pi->size() < expected)
        return;
    
    // set_track_count(num_tracks);
    uint16_t drum_id;
    for(int i=0; i<num_tracks; i++) {
        pi->read(&drum_id, sizeof(uint16_t));
        tstate[i].drumId = drum_id;
        set_voice(i, drum_id);
    }
}

void DrumPlugin::set_voice(int index, uint16_t drum_id) {
    // can only insert at the back or replace a current vector element
    // fail if trying to insert two ot three places past the end
    if(index < track_voices.size() && track_voices[index]->matches(drum_id))
        return;
    else if (index > track_voices.size())
        return;

    auto voice_it = find_inactive_voice(drum_id);

    if(voice_it != inactive_voices.end()) {
        set_voice(index, *voice_it);
        inactive_voices.erase(voice_it);
        return;
    } 
    
    auto voice = drumSets->getDrumVoice(drum_id);
    if(voice != nullptr) {
        set_voice(index, voice);
    }
}

void DrumPlugin::set_voice(int index, DrumVoice* voice) {
    if(index == track_voices.size()) {       // inserting a new voice at the end
        return track_voices.push_back(voice);     //  no need to move a voice to active/inactive voices 
    }

    if(track_voices[index]->is_playing())
        active_voices.push_back(track_voices[index]);
    else if (find_inactive_voice(voice) != inactive_voices.end()) { // a copy of this drum already in inactive)vioces
        delete track_voices[index];                                 // only want one copy of each drum, so delete spare
    } else {
        inactive_voices.push_back(track_voices[index]);
    }

    track_voices[index] = voice;
}

std::vector<DrumVoice*>::iterator DrumPlugin::find_inactive_voice(uint16_t drum_id) {
    for (auto it = inactive_voices.begin(); it != inactive_voices.end(); it++)
        if((*it)->matches(drum_id))
            return it;

    return inactive_voices.end();
}

std::vector<DrumVoice*>::iterator DrumPlugin::find_inactive_voice(DrumVoice* match_voice) {
    for (auto it = inactive_voices.begin(); it != inactive_voices.end(); it++)
        if((*it) == match_voice)
            return it;

    return inactive_voices.end();
}

void DrumPlugin::set_track_count(int new_track_count) {
    if(static_cast<uint16_t>(new_track_count) >= track_voices.size()) {
        for(int add_index = (int)track_voices.size(); add_index < new_track_count; add_index++)
            set_voice(add_index, (uint16_t) 0);
    }

    if(new_track_count > track_count) {
        for(int i = track_count; i < new_track_count; i++) {
            tstate[i].reset();
        }
    } else if(new_track_count < track_count) {
        for(int i = new_track_count; i < track_count; i++)
            track_voices[i]->stopNote(false);
    }

    track_count = new_track_count;
}




bool DrumPlugin::process_stereo(float **pin, float **pout, int numsamples, int mode) {
    if(mode != zzub::process_mode_write) {
        return true;
    }

    for(auto voice: track_voices)
        voice->renderNextBlock(pout[0], numsamples);

    for(auto it = active_voices.begin(); it != active_voices.end(); ) {
        (*it)->renderNextBlock(pout[0], numsamples);
        if(!(*it)->is_playing()) {
            inactive_voices.push_back(*it);
            active_voices.erase(it);
        } else {
            ++it;
        }
    }

    memcpy(pout[1], pout[0], numsamples * sizeof(float));
    return true;
}


void DrumPlugin::process_events() {
    for(auto it = active_voices.begin(); it != active_voices.end(); ) {
        if(!(*it)->is_playing()) {
            if(find_inactive_voice(*it) == inactive_voices.end()) 
                inactive_voices.push_back(*it);
            else
                delete *it;
            it = active_voices.erase(it);
        } else {
            ++it;
        }
    }

    for(int i=0; i<track_count; i++) {
        if(tval[i].volume != 0xff) {
            tstate[i].volume = tval[i].volume;
        }

        if(tval[i].drumId != TRACKVAL_NO_DRUM && tstate[i].drumId != tval[i].drumId && !track_voices[i]->matches(tval[i].drumId)) {
            tstate[i].drumId = tval[i].drumId;
            set_voice(i, tval[i].drumId);
        }

        if(tval[i].stretch != TRACKVAL_NO_TIMESTRETCH ) {
            tstate[i].stretch = tval[i].stretch;
        }

        if(tval[i].note != zzub::note_value_none) {
            if(tval[i].note == zzub::note_value_off) {
                tstate[i].note = zzub::note_value_off;
                track_voices[i]->stopNote(true);
            } else {
                tstate[i].note = tval[i].note;
                track_voices[i]->startNote(tval[i].note, tstate[i].amplitude(), _master_info->samples_per_second, tstate[i].timestretch());
            }
        }
    }
}


const char * DrumPlugin::describe_value(int param, int value) {
    static char desc[32];

    switch(param % 4) {
    case 0:
    case 1:
        return "";

    case 2:{
        auto preset = drumSets->getDrumPreset(value);

        if(preset == nullptr) {
            return "--- not a drum ---";
        } else {
            snprintf(desc, 32, "%s | %s", preset->drumName.c_str(), preset->drumsetName.c_str());
            return desc;    
        }
    }
    case 3:
        snprintf(desc, 32, "%.3f%%", value / 2.560);
        return desc;
    }
}

zzub::plugin* DrumPluginInfo::create_plugin() const { return new DrumPlugin(drumSets); }


DrumPluginInfo::DrumPluginInfo(DrumSets* drumSets) : drumSets(drumSets) {
    this->flags = zzub::plugin_flag_has_audio_output | zzub::plugin_flag_is_instrument;
    this->min_tracks = 1;
    this->max_tracks = 8;
    this->name = "mda Drum";
    this->short_name = "mda Drum";
    this->author = "";
    this->uri = "@libneil/mda/generator/Drum";

    add_track_parameter().set_note();

    add_track_parameter().set_byte()
                         .set_name("Volume")
                         .set_description("Volume");

    add_track_parameter().set_word()
                         .set_name("Drum")
                         .set_description("Drum")
                         .set_flags(zzub::parameter_flag_state)
                         .set_value_none(TRACKVAL_NO_DRUM)
                         .set_value_default(TRACKVAL_NO_DRUM);

    add_track_parameter().set_word()
                         .set_name("Stretch")
                         .set_description("Stretch")
                         .set_value_default(TRACKVAL_DEFAULT_TIMESTRETCH)
                         .set_flags(zzub::parameter_flag_state);
}

zzub::plugincollection *zzub_get_plugincollection() {
    return new DrumPluginCollection();
}

const char *zzub_get_signature() {
    return ZZUB_SIGNATURE;
}
