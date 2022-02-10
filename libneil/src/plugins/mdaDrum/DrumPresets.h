#pragma once

#include <vector>
#include "DrumDefines.h"
#include "utils/tinydir.h"
#include "DrumVoice.h"
#include <string>

#include <map>

struct DrumPreset {
    std::string drumName;
    std::string drumsetName;
    float params[COUNT_PRESET_PARAMS] {0};

    DrumPreset(std::string drumName, std::string setName) : drumName(drumName), drumsetName(setName) {}
};


struct DrumSet {
    std::string name;

    DrumSet(std::string name);
    DrumSet(DrumPreset *preset);

    void add(DrumPreset *);
    size_t size() const { return drums.size(); }
    DrumPreset* getDrumPreset(uint8_t pos);

private:
    std::vector<DrumPreset*> drums{};
};


struct DrumSets {
public:
    DrumSets(std::string dir, uint16_t block_size);

    void add(DrumPreset* preset);
    DrumVoice* getDrumVoice(uint8_t drumset_pos, uint8_t drum_pos) const;
    DrumVoice* getDrumVoice(uint16_t drum_id) const;
    DrumPreset* getDrumPreset(uint16_t drum_id) const;
    DrumPreset* getDrumPreset(uint8_t drumset_pos, uint8_t drum_pos) const;
    DrumSet* getDrumSet(std::string name);

    size_t size() const;
    void loadPresetsIn(std::string base_path);

private:
    uint16_t block_size;
    std::vector<DrumSet*> sets{};
    DrumPreset *importPreset(std::string full_path, std::string drumset_name, std::string drum_path);
    void importDir(std::string curr_path, std::string base_path);
    // envelopes are defined by comma separated on a single line. 
    // each pair consists of a a time and an amplitude separated by a comma.
    // each time,amp pair is separated from the next pair with a space
    void readEnvelope(DrumPreset *ps, const int parameterOffset, std::string envelope);
    DrumVoice* getDrum(uint8_t set, uint8_t drum);
};
