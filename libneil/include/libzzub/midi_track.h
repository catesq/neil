#pragma once

#include <cstdint>

#include "zzub/plugin.h"

#include "loguru.hpp"
#include <functional>

#define SAMPLES_TO_BEATS(sample_pos, sample_rate, bpm) ((sample_pos * bpm) / (sample_rate * 60.f))


inline void LOG_BEAT(std::string tag, zzub::master_info* master_info, uint64_t sampl_pos) {
    LOG_F(INFO, "%s: at beat %.2f\n", tag.c_str(), SAMPLES_TO_BEATS(sampl_pos, master_info->samples_per_second, master_info->beats_per_minute));
}

namespace zzub {

extern const std::string midi_note_names[12]; 

enum zzub_note_change {
	zzub_note_change_none = 0,
	zzub_note_change_noteoff = 1,
	zzub_note_change_noteon = 2,
	zzub_note_change_volume = 3
};


enum zzub_note_len {
	zzub_note_len_none = 0,
	zzub_note_len_default = 1,
	zzub_note_len_min = 1,
	zzub_note_len_max = 65535
};


enum zzub_note_unit {
	zzub_note_unit_none = 255,
	zzub_note_unit_min = 0,
	zzub_note_unit_max = 5,
	zzub_note_unit_default = 0,
	zzub_note_unit_beats = 0,
	zzub_note_unit_beats_16ths = 1,
	zzub_note_unit_beats_256ths = 2,
	zzub_note_unit_secs = 3,
	zzub_note_unit_secs_16ths = 4,
	zzub_note_unit_secs_256ths = 5
};


enum zzub_midi_command {
	zzub_midi_command_none = 0,
	zzub_midi_command_default = 0,
	zzub_midi_command_min = 1,
	zzub_midi_command_max = 255,
	zzub_midi_command_length = 1
};


enum zzub_midi_data {
	zzub_midi_data_none = 65535,
	zzub_midi_data_default = 65535,
	zzub_midi_data_min = 0,
	zzub_midi_data_max = 128
};

enum zzub_midi_track_param {
    zzub_midi_track_param_note = 0,
    zzub_midi_track_param_volume,
    zzub_midi_track_param_note_unit,
    zzub_midi_track_param_note_len,
    zzub_midi_track_param_command,
    zzub_midi_track_param_data
};



#pragma pack(1)

// used to send midi command data sent to the plugin
union midi_cmd_data {
    uint8_t bytes[3]{};
    struct {
        uint8_t cmd;
        uint16_t data;
    } midi;
};

// used internally in the track manager
struct midi_note_len {
    uint8_t unit;
    uint16_t length;

    bool is_valid() const {
        return unit != zzub_note_unit_none && length != zzub_note_len_none;
    }
};

struct midi_note_track {
    uint8_t note = zzub_note_value_none;
    uint8_t volume = zzub_volume_value_none;
    
    uint8_t unit = zzub_note_unit_none;
    uint16_t length = zzub_note_len_none;

    uint8_t command = zzub_midi_command_none;
    uint16_t data = zzub_midi_data_none;

    uint8_t get_note();

    // 0 - 127
    int get_volume();
    bool is_note_on();
    void set_note_off();
    bool is_volume_on();
    int note_change_from(midi_note_track &prev);
};

#pragma pack()

namespace {
    
// the track_manager keeps a list of these active notes
struct active_note {
    uint16_t note;
    uint16_t track_num;
    uint64_t start_at;
    uint64_t length;

    bool has_ended_at(uint64_t tick) const {
        return tick >= start_at + length;
    }
};


}  // end private namespace



static const midi_note_len invalid_note_len{zzub_note_unit_none, zzub_note_len_none};



// implemented by the zzub::plugin by the lv2 and vst adapters, they use different data structures to send midi messages to the lv2/vst plugin being hosted
// but need the same midi data from the plugin  
// the plugin adapter which implements this interface must call midi_track_manager::process_events()
// process_events reads the current midi track data of the adapter then calls "add_..." functions on the adapter  
struct midi_plugin_interface {
    virtual void add_note_on(uint8_t note, uint8_t volume) = 0;
    virtual void add_note_off(uint8_t note) = 0;
    virtual void add_aftertouch(uint8_t note, uint8_t volume) = 0;
    virtual void add_midi_command(uint8_t cmd, uint8_t data1, uint8_t data2) = 0;
};



inline std::string describe_note_len(int note_len_type) {
    switch(note_len_type) {
        case zzub_note_unit_beats:
            return "beats";

        case zzub_note_unit_beats_16ths:
            return "beats/16";

        case zzub_note_unit_beats_256ths:
            return "beats/256";

        case zzub_note_unit_secs:
            return "sec";

        case zzub_note_unit_secs_16ths:
            return "secs/16";

        case zzub_note_unit_secs_256ths:
            return "secs/256";

        default:
            return "no note length";
    }
}




// used by the lv2 and vst plugins to process midi notes, volume, note length and midi data/commands
struct midi_track_manager {
    static const uint16_t default_max_tracks = 16;

private:
    midi_plugin_interface &plugin;

    // the maximum number of tracks
    uint16_t max_num_tracks;

    std::vector<midi_note_track> curr_tracks{};

    std::vector<midi_note_track> prev_tracks{};

    std::vector<active_note> active_notes{};

    uint32_t num_tracks = 0;

    // used a counter to send note length events
    uint64_t play_pos = 0;

    uint32_t sample_rate = zzub_default_rate;

    float bpm = 126.0f;

public:
    midi_track_manager(midi_plugin_interface &plugin)
        : midi_track_manager(plugin, default_max_tracks) 
    {
    }

    midi_track_manager(midi_plugin_interface &plugin, uint16_t max_num_tracks)
        : plugin(plugin), max_num_tracks(max_num_tracks), curr_tracks(max_num_tracks)
    {
    }


    ~midi_track_manager() 
    {
    }


    // called in zzub::info for the vst/lv2 adapter - adds the track parameters to the plugin
    static void add_midi_track_info(zzub::info *info);



    void init(uint32_t rate) ;


    // called in the process_events method of a plugin
    void process_events();


    void process_samples(uint16_t numsamples, int mode);


    // when the plugin receives a event_type_edit_pattern event the new value is forwarded here
    // needed to remember the note length unit while editing
    // the prev_state method only works while playing songs 
    void parameter_update(int track, int column, int row, int value);


    void set_track_count(int new_num_tracks);

 
    std::string describe_value(int track, int param, int value);


    void* get_track_data() const {
        return (void *) &curr_tracks[0];
    }


    void set_sample_rate(uint32_t rate) {
        sample_rate = rate;
    }


    void set_bpm(uint32_t bpm) {
        this->bpm = bpm;
    }


    uint32_t get_max_num_tracks() const {
        return max_num_tracks;
    }
private:

    std::string describe_note_len(int note_len_type, int value);


    // is this note measured in beats/samples/seconds
    const zzub_note_unit get_curr_unit(const midi_note_track *curr, const midi_note_track *prev) const;

    // how many beats/sample/seconds
    uint16_t get_curr_len(const midi_note_track *curr, const midi_note_track *prev) const;
    
    // how many beats/sample/seconds and which
    midi_note_len get_midi_note_len(const midi_note_track *curr, const midi_note_track *prev) const;

    // beats per minute converted to samples per second
    inline float get_beat_len() {
        return (60.0f * sample_rate) / bpm;
    }


    uint64_t get_note_len_in_samples(midi_note_len note_len);

    bool remove_matching_notes(std::function<bool(const active_note&)> matcher);
};






inline std::string midi_chan(uint8_t byte) { return std::to_string(byte & 0x0f); }


inline std::string midi_note(uint8_t byte) {
    return std::string(midi_note_names[byte % 12]) + std::to_string(byte / 12 - 1);
}

inline std::string midi_note(uint8_t* data, uint8_t size) {
    if (size == 3) 
        return midi_note(data[1]) + "(" + std::to_string(data[2]) + ")";
    else
        return midi_note(data[1]);
}

}