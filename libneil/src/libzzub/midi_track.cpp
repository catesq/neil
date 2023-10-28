
#include "libzzub/midi_track.h"
#include "zzub/plugin.h"

#include "loguru.hpp"
#include "libzzub/tools.h"


namespace zzub {

const std::string midi_note_names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

int midi_note_track::note_change_from(midi_note_track &prev) 
{                                                               
    if (note == zzub::note_value_none) {                                                               
        if (is_volume_on() && volume != prev.volume) {                                                 
            return zzub_note_change_volume;                                                            
        } else {                                                                                       
            return zzub_note_change_none;                                                              
        }                                                                                              
    }                                                                                                  

    if (note == zzub::note_value_off) {                                                                
        return zzub_note_change_noteoff;                                                               
    } else {                                                                                           
        return zzub_note_change_noteon;                                                                
    }                                                                                                  
}




void midi_track_manager::init(uint32_t rate) 
{
    set_sample_rate(rate);

    for (auto idx = 0; idx < max_num_tracks; ++idx) {
        auto curr_ptr = plugin.get_track_data_pointer(idx);

        if (!curr_ptr) {
            throw std::runtime_error("midi_track_manager::init - plugin.get_track_data_pointer returned null for track " + std::to_string(idx));
        }

        // clear the midi note track data in curr[idx]
        curr_ptr->note = zzub_note_value_none;
        curr_ptr->volume = zzub_volume_value_none;
        curr_ptr->unit = zzub_note_unit_none;
        curr_ptr->length = zzub_note_len_none;
        curr_ptr->command = zzub_midi_command_none;
        curr_ptr->data = zzub_midi_data_none;

        curr_tracks.push_back(curr_ptr);
    }
}




void midi_track_manager::set_track_count(int new_num_tracks) 
{
    if(new_num_tracks > max_num_tracks)
        return;

    prev_tracks.resize(new_num_tracks);

    num_tracks = new_num_tracks;
}


// will handle the note length messages
void midi_track_manager::process_samples(uint16_t numsamples, int mode) 
{
    if(mode == zzub_process_mode_no_io) {
        return;
    }

    play_pos += numsamples;
    auto it = active_notes.begin();

    // iterate over active notes and remove any note's which have ended
    while(it != active_notes.end()) {
        auto &active = *it;

        if ( active.has_ended_at(play_pos) ) {
            auto track = &prev_tracks[active.track_num];
            plugin.add_note_off(track->note);
            track->set_note_off();
            it = active_notes.erase(it);
        } else {
            ++it;
        }
    }
}


// called in the process_events method of a plugin
void midi_track_manager::process_events() 
{
    for (uint16_t track_num = 0; track_num < num_tracks; track_num++) {
        auto prev = &prev_tracks[track_num];
        auto curr = curr_tracks[track_num];

        uint8_t volume;
        uint64_t samp_len;

        switch (curr->note_change_from(*prev)) {
            case zzub_note_change_none:
                break;

            case zzub_note_change_noteoff: {
                if (!prev->is_note_on())
                    break;

                auto it = std::find_if(active_notes.begin(), active_notes.end(), [track_num, prev](const active_note &active) {
                    return active.track_num == track_num;
                });

                if (it != active_notes.end()) {
                    active_notes.erase(it);
                    plugin.add_note_off(prev->note);
                    prev->set_note_off();
                }

                break;
            }
            
            case zzub_note_change_noteon: {
                if (curr->is_volume_on())
                    prev->volume = curr->volume;

                prev->note = curr->note;

                if (prev->is_volume_on()) 
                    volume = prev->volume;
                else
                    volume = zzub_volume_value_default;

                auto note_len = get_note_length(prev, curr);

                if(note_len.is_valid()) {
                    samp_len = get_note_len_in_samples(note_len);
                } else {
                    samp_len = get_beat_length();
                }

                plugin.add_note_on(curr->note, volume);
                active_notes.emplace_back(curr->note, track_num, play_pos, samp_len);
                
                break;
            }

            case zzub_note_change_volume:
                prev->volume = curr->volume;

                if (prev->is_note_on())
                    plugin.add_aftertouch(curr->note, prev->volume);

                break;
        }
    }
}

inline std::string describe_note_len_unit(int note_len_type) {
    switch(note_len_type) {
        case zzub_note_unit_beats:
            return "beats";

        case zzub_note_unit_beats_16ths:
            return "beats/16";

        case zzub_note_unit_beats_256ths:
            return "beats/256";

        case zzub_note_unit_secs:
            return "secs";

        case zzub_note_unit_secs_16ths:
            return "secs/16";

        case zzub_note_unit_secs_256ths:
            return "secs/256";

        default:
            return "unknown";
    }
} 


std::string midi_track_manager::describe_value(int track, int param, int value) 
{
    param = param % 5;
    auto& state = prev_tracks[track];

    switch(param) {
        case 0:
            return ""; // note descriptions are already handled 

        case 1:
            return std::to_string(value);

        case 2:
            return describe_note_len_unit(value);

        case 3: 
            if(prev_tracks[track].unit != zzub_note_unit_none) {
                return describe_note_len(prev_tracks[track].unit, value);
            } else {
                return describe_note_len(zzub_note_unit_default, value);
            }

        case 4:
            return "command: " + std::to_string(value);

        case 5:
            return "data: " + std::to_string(value);

        default:
            return "unknown param: " + std::to_string(value);
    }
}


std::string midi_track_manager::describe_note_len(int note_len_unit, int value) 
{
    switch(note_len_unit) {
        case zzub_note_unit_beats:
            return std::to_string(value) + " beats";

        case zzub_note_unit_beats_16ths:
            return std::to_string((int)(value / 16)) + " " + std::to_string(value % 16) + "/16 beats";

        case zzub_note_unit_beats_256ths:
            return std::to_string((int)(value / 256)) + " " + std::to_string(value % 16) + "/256 beats";

        case zzub_note_unit_secs:
            return std::to_string(value) + " sec";

        case zzub_note_unit_secs_16ths:
            return std::to_string(value * 62.5) + " ms";

        case zzub_note_unit_secs_256ths:
            return std::to_string(value * 3.90625) + " ms";

        default:
            return "unknown";
    }
}



uint64_t midi_track_manager::get_note_len_in_samples(midi_note_len note_len) 
{
    switch(note_len.unit) {
        case zzub_note_unit_secs:
            return sample_rate * note_len.length;

        case zzub_note_unit_secs_16ths:
            return sample_rate * (note_len.length / 16.0f);

        case zzub_note_unit_secs_256ths:
            return sample_rate * (note_len.length / 256.0f);

        case zzub_note_unit_beats:
            return get_beat_length() * note_len.length;

        case zzub_note_unit_beats_16ths:
            return get_beat_length() * (note_len.length / 16.0f);

        case zzub_note_unit_beats_256ths:
            return get_beat_length() * (note_len.length / 256.0f);

        default:
            return sample_rate;
    }
}

const zzub_note_unit midi_track_manager::get_note_unit(const midi_note_track *prev, const midi_note_track *curr) const 
{
    if (curr->unit != zzub_note_unit_none) {
        return static_cast<zzub_note_unit>(curr->unit);
    } else if (prev->unit != zzub_note_unit_none) {
        return static_cast<zzub_note_unit>(prev->unit);
    } else {
        return static_cast<zzub_note_unit>(zzub_note_len_none);
    }
}

midi_note_len midi_track_manager::get_note_length(const midi_note_track *prev, const midi_note_track *curr) const 
{
    if(curr->length == zzub_note_len_none) {
        return invalid_note_len;
    } else {
        return midi_note_len {
            static_cast<uint8_t>(get_note_unit(prev, curr)), 
            curr->length
        };
    }
}


void midi_track_manager::add_midi_track_info(zzub::info* info) 
{
        info->min_tracks = 1;
        info->max_tracks = midi_track_manager::default_max_tracks;

        info->add_track_parameter()
             .set_note();

        info->add_track_parameter()
             .set_byte()
             .set_name("Volume")
             .set_description("Volume (00-7f)")
             .set_value_min(zzub_volume_value_min)
             .set_value_max(zzub_volume_value_max)
             .set_value_none(zzub_volume_value_none)
             .set_value_default(zzub_volume_value_default);

        info->add_track_parameter()
             .set_byte()
             .set_name("Note length unit")
             .set_description("Note length options: 0=beats, 1=beats/16, 2=secs, 3=secs/16, 4=secs/256")
             .set_value_min(zzub_note_unit_min)
             .set_value_max(zzub_note_unit_max)
             .set_value_none(zzub_note_unit_none)
             .set_value_default(zzub_note_unit_default);

        info->add_track_parameter()
             .set_word()
             .set_name("Note length")
             .set_description("Length")
             .set_value_min(zzub_note_len_min)
             .set_value_max(zzub_note_len_max)
             .set_value_none(zzub_note_len_none)
             .set_value_default(zzub_note_len_default);

        info->add_track_parameter()
             .set_byte()
             .set_name("Command")
             .set_description("Command")
             .set_value_min(zzub_midi_command_min)
             .set_value_max(zzub_midi_command_max)
             .set_value_none(zzub_midi_command_none)
             .set_value_default(zzub_midi_command_none);

        info->add_track_parameter()
             .set_word()
             .set_name("Data")
             .set_description("Data")
             .set_value_min(zzub_midi_data_min)
             .set_value_max(zzub_midi_data_max)
             .set_value_none(zzub_midi_data_none)
             .set_value_default(zzub_midi_data_none);
    }

}