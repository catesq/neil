
#include "zzub/plugin.h"
#include "libzzub/midi_track.h"
#include "libzzub/tools.h"


namespace zzub {


const std::string midi_note_names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };



uint8_t midi_note_track::get_note() 
{ 
    return note; 
}



int midi_note_track::get_volume() 
{ 
    return volume != zzub_volume_value_none ? volume : zzub_volume_value_default; 
}



bool midi_note_track::is_note_on() 
{ 
    return note != zzub_note_value_none && note != zzub_note_value_off; 
}



void midi_note_track::set_note_off() 
{
    note = zzub_note_value_off;
}



bool midi_note_track::is_volume_on() 
{ 
    return volume != zzub_volume_value_none; 
}



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
}



void midi_track_manager::set_track_count(int new_num_tracks) 
{
    if(new_num_tracks > max_num_tracks)
        return;

    prev_tracks.resize(new_num_tracks);

    num_tracks = new_num_tracks;
}



// will handle the note length messages
void midi_track_manager::process_samples(
    uint16_t numsamples, 
    int mode
) 
{
    if(mode == zzub_process_mode_no_io) {
        return;
    }

    play_pos += numsamples;

    remove_matching_notes([this](const active_note &active) {
        return active.has_ended_at(play_pos);
    });
}



void midi_track_manager::parameter_update(
    int track, 
    int column, 
    int row,
    int value
) {
    auto &prev = prev_tracks[track];

    if(column == zzub_midi_track_param_note_unit && value != zzub_note_unit_none) {
        prev.unit = value;
    }
}



// called in the process_events method of a plugin
void midi_track_manager::process_events() 
{
    for (uint16_t track_num = 0; track_num < num_tracks; track_num++) {
        
        auto prev = &prev_tracks[track_num];
        auto curr = &curr_tracks[track_num];

        uint8_t volume;
        uint64_t samp_len;

        switch (curr->note_change_from(*prev)) {
            case zzub_note_change_none:
                break;

            case zzub_note_change_noteoff: {
                if (!prev->is_note_on())
                    break;
                
                remove_matching_notes([track_num](const active_note &active) {
                    return active.track_num == track_num;
                });

                break;
            }
            
            case zzub_note_change_noteon: {
                remove_matching_notes([curr](const active_note &active) {
                    return active.note == curr->note;
                });

                if (curr->is_volume_on())
                    prev->volume = curr->volume;

                prev->note = curr->note;

                if (prev->is_volume_on()) 
                    volume = prev->volume;
                else
                    volume = zzub_volume_value_default;

                auto note_len = get_midi_note_len(prev, curr);

                if(note_len.is_valid()) {
                    samp_len = get_note_len_in_samples(note_len);
                } else {
                    samp_len = get_beat_len();
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

        if(curr->unit != zzub_note_unit_none) {
            prev->unit = curr->unit;
        }

        if(curr->length != zzub_note_len_none) {
            prev->length = curr->length;
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



std::string midi_track_manager::describe_value(
    int track, 
    int param, 
    int value
) 
{
    param = param % 5;
    auto& state = prev_tracks[track];

    switch(param) {
        case zzub_midi_track_param_note:
            return ""; // note descriptions are already handled 

        case zzub_midi_track_param_volume:
            return std::to_string(value);

        case zzub_midi_track_param_note_unit:
            return describe_note_len_unit(value);

        case zzub_midi_track_param_note_len: 
            if(prev_tracks[track].unit != zzub_note_unit_none) {
                return describe_note_len(prev_tracks[track].unit, value);
            } else {
                return describe_note_len(zzub_note_unit_default, value);
            }

        case zzub_midi_track_param_command:
            return "command: " + std::to_string(value);

        case zzub_midi_track_param_data:
            return "data: " + std::to_string(value);

        default:
            return "unknown param: " + std::to_string(value);
    }
}



std::string midi_track_manager::describe_note_len(
    int note_len_unit, 
    int value
) 
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
            return get_beat_len() * note_len.length; 

        case zzub_note_unit_beats_16ths:
            return get_beat_len() * (note_len.length / 16.0f);

        case zzub_note_unit_beats_256ths:
            return get_beat_len() * (note_len.length / 256.0f);

        default:
            return sample_rate;
    }
}



const zzub_note_unit midi_track_manager::get_curr_unit(
    const midi_note_track *prev,
    const midi_note_track *curr
) const 
{
    if (curr->unit != zzub_note_unit_none) {
        return static_cast<zzub_note_unit>(curr->unit);
    } else if (prev->unit != zzub_note_unit_none) {
        return static_cast<zzub_note_unit>(prev->unit);
    } else {
        return static_cast<zzub_note_unit>(zzub_note_unit_default);
    }
}

uint16_t midi_track_manager::get_curr_len(
    const midi_note_track *prev, 
    const midi_note_track *curr
) const
{
    if (curr->length != zzub_note_len_none) {
        return curr->length;
    } else if (prev->length != zzub_note_len_none) {
        return prev->length;
    } else {
        return zzub_note_len_default;
    }
}


midi_note_len midi_track_manager::get_midi_note_len(
    const midi_note_track *prev, 
    const midi_note_track *curr
) const 
{
    auto unit = get_curr_unit(prev, curr);
    auto length = get_curr_len(prev, curr);

    return midi_note_len { static_cast<uint8_t>(unit), length };
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
        .set_description("Volume (00-80)")
        .set_value_min(zzub_volume_value_min)
        .set_value_max(zzub_volume_value_max)
        .set_value_none(zzub_volume_value_none)
        .set_value_default(zzub_volume_value_default);

    info->add_track_parameter()
        .set_byte()
        .set_name("Note length unit")
        .set_description("0:beats | 1:beats/16 | 2:beats/256 | 4:secs | 5:secs/16 | 6:secs/256")
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






// @param port_offset: how many ports does the plugin already have
std::vector<zzub::port*> midi_track_manager::build_midi_zzub_ports(zzub::info* info, int port_offset) {
    return {
        new track_port(this, "Note", port_offset + 0, zzub_midi_track_param_note, offsetof(midi_note_track, note), info->track_parameters[zzub_midi_track_param_note]),
        new track_port(this, "Volume", port_offset + 1, zzub_midi_track_param_volume, offsetof(midi_note_track, volume), info->track_parameters[zzub_midi_track_param_volume]),
        new track_port(this, "Note unit", port_offset + 2, zzub_midi_track_param_note_unit, offsetof(midi_note_track, unit), info->track_parameters[zzub_midi_track_param_note_unit]),
        new track_port(this, "Note length", port_offset + 3, zzub_midi_track_param_note_len, offsetof(midi_note_track, length), info->track_parameters[zzub_midi_track_param_note_len]),
        new track_port(this, "Command", port_offset + 4, zzub_midi_track_param_command, offsetof(midi_note_track, command), info->track_parameters[zzub_midi_track_param_command]),
        new track_port(this, "Data", port_offset + 5, zzub_midi_track_param_data, offsetof(midi_note_track, data), info->track_parameters[zzub_midi_track_param_data])
    };
}


bool midi_track_manager::remove_matching_notes(std::function<bool(const active_note&)> note_matcher) {
    int removed = 0;

    auto it = active_notes.begin();

    while(it != active_notes.end()) {
        auto &playing = *it;

        if ( note_matcher(playing) ) {
            plugin.add_note_off(playing.note);
            prev_tracks[playing.track_num].set_note_off();
            it = active_notes.erase(it);
            removed += 1;
        } else {
            ++it;
        }
    }

    return removed > 0;
}


}
