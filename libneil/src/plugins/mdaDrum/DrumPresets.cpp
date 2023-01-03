#include "DrumPresets.h"
#include "DrumVoice.h"

#include "utils/tinydir.h"
#include "utils/iniparser.h"
#include <string>
#include <typeinfo>
#include <boost/locale.hpp>
#include <boost/algorithm/string/case_conv.hpp>


DrumKit::DrumKit(std::string name) : name(name) {}
DrumKit::DrumKit(DrumPreset *preset) : name(preset->drumsetName) {
    add(preset);
}


void DrumKit::add(DrumPreset *preset) {
    drums.push_back(preset);
}


DrumPreset* DrumKit::getDrumPreset(uint8_t pos) {
    if(pos < drums.size())
        return drums[pos];
    else
        return nullptr;
}


DrumKits::DrumKits(std::string dir, uint16_t block_size): block_size(block_size) {
    loadPresetsIn(dir);
}

DrumKit* DrumKits::getDrumKit(std::string name) {
    for(auto set: sets)
        if(set->name == name)
            return set;

    return nullptr;
}

void DrumKits::add(DrumPreset* preset) {
    auto set = getDrumKit(preset->drumsetName);

    if(set != nullptr)
        set->add(preset);
    else 
        sets.emplace_back(new DrumKit(preset));
}

DrumPreset* DrumKits::getDrumPreset(uint8_t drumset_pos, uint8_t drum_pos) const {
    if(drumset_pos >= sets.size())
        return nullptr;

    return sets[drumset_pos]->getDrumPreset(drum_pos);
}

DrumPreset* DrumKits::getDrumPreset(uint16_t drum_id) const {
    return getDrumPreset(drum_id >> 8, drum_id & 0xFF);
}

DrumVoice* DrumKits::getDrumVoice(uint16_t drum_id) const {
    return getDrumVoice(drum_id >> 8, drum_id & 0xFF);    
}

DrumVoice* DrumKits::getDrumVoice(uint8_t drumset_id, uint8_t drum_id) const {
    DrumPreset* preset;

    if((preset=getDrumPreset(drumset_id, drum_id)) != nullptr)
        return new DrumVoice((drumset_id << 8) | drum_id, preset->params, block_size);
    else
        return nullptr;
}


size_t DrumKits::size() const {
    return sets.size();
}


void DrumKits::loadPresetsIn(std::string base_path) {
    importDir(base_path, std::string("/")); 

    std::sort(sets.begin(), sets.end(), [] (DrumKit* setA, DrumKit *setB) {
        return boost::to_lower_copy(std::string(setA->name)) < 
               boost::to_lower_copy(std::string(setB->name));
    });
}


void DrumKits::importDir(std::string path, std::string base_path) {
    tinydir_dir dir;
    tinydir_open(&dir, path.c_str());

    while(dir.has_next) {
        tinydir_file file;
        tinydir_readfile(&dir, &file);

        std::string full_path = path + "/" + file.name;

        if(strcmp(file.name, ".") == 0 || strcmp(file.name, "..") == 0) {
        } else if(file.is_dir) {
            importDir(full_path, file.name);
        } else if (file.is_reg && (strcmp(file.extension, "ds") == 0 || strcmp(file.extension, "dsfile") == 0)) {
            DrumPreset *preset = importPreset(
                full_path,
                base_path,
                std::string(file.name).substr(0, strlen(file.name) - strlen(file.extension) - 1)
            );

            if(preset != nullptr) {
                add(preset);
            }
        }

        tinydir_next(&dir);
    }

    tinydir_close(&dir);
}


void DrumKits::readEnvelope(DrumPreset *ps, const int parameterOffset, std::string envelope) {
    int currentPos = 0, currentEnvelopePoint = 0, currentParameter = parameterOffset;
    int point = 0;

    while(currentPos < envelope.length()) {
        int nextPos = envelope.find(",", currentPos);

        if(nextPos == std::string::npos) {
            break;
        }

        std::string timeValue = envelope.substr(currentPos, nextPos - currentPos);

        currentPos = nextPos + 1;

        nextPos = envelope.find(" ", currentPos);
        std::string gainValue = envelope.substr(currentPos, nextPos == std::string::npos ? envelope.length() - currentPos: nextPos - currentPos);

        ps->params[currentParameter++] = std::stof(timeValue) / 44100.f;
        ps->params[currentParameter++] = std::stof(gainValue);

        currentPos = nextPos + 1;

        if(++currentEnvelopePoint == MAX_ENV_POINTS) {
            return;
        } else if (nextPos == std::string::npos) {
            break;
        }
    }

    if(currentEnvelopePoint == 0) {
        ps->params[0] = 0.f; ps->params[1] = 1.f;
        ps->params[2] = 1.f; ps->params[3] = 1.f;
        ps->params[4] = 1.1f;
        ps->params[5] = 0.f;
        ps->params[6] = -1.f;

        return;
    }

    while(++currentEnvelopePoint < MAX_ENV_POINTS) {
        ps->params[currentParameter++] = -1.f;
        ps->params[currentParameter++] = 0.f;
    }
}


DrumPreset* DrumKits::importPreset(std::string full_path, std::string drumset_name, std::string drum_name) {
    dictionary* ini;
//    printf("%s %s %s\n", drumset_name.c_str(), drum_name.c_str(), full_path.c_str());

    ini = iniparser_load(full_path.c_str());

    if(ini == 0) {
        return nullptr;
    }

    DrumPreset *ps = new DrumPreset(drum_name, drumset_name);


    // String version = iniparser_getstring(ini, "general:version", "DrumSynth v1.0");
    // String comment = iniparser_getstring(ini, "general:comment", "None");

    ps->params[PP_MAIN_TUNING] = (float) iniparser_getdouble(ini, "general:tuning", 0.0);
    ps->params[PP_MAIN_STRETCH] = (float) iniparser_getdouble(ini, "general:stretch", 100.0);
    ps->params[PP_MAIN_GAIN] = (float) iniparser_getdouble(ini, "general:level", 0.0);
    ps->params[PP_MAIN_FILTER] = (float) iniparser_getint(ini, "general:filter", 0);
    ps->params[PP_MAIN_HIGHPASS] = (float) iniparser_getint(ini, "general:highpass", 0);
    ps->params[PP_MAIN_RESONANCE ] = (float) iniparser_getint(ini, "general:resonance", 0);
    readEnvelope(ps, PP_MAIN_ENV_T1TIME, iniparser_getstring(ini, "general:filterenv", "0,100 44100,100 44200,0"));

    ps->params[PP_TONE_ON] = (float) iniparser_getint(ini, "tone:on", 1);
    ps->params[PP_TONE_LEVEL] = (float) iniparser_getint(ini, "tone:level", 128);
    ps->params[PP_TONE_F1] = (float) iniparser_getdouble(ini, "tone:f1", 200.0);
    ps->params[PP_TONE_F2] = (float) iniparser_getdouble(ini, "tone:f2", 50.0);
    ps->params[PP_TONE_DROOP] = (float) iniparser_getdouble(ini, "tone:droop", 50.0);
    ps->params[PP_TONE_PHASE] = (float) iniparser_getdouble(ini, "tone:phase", 0.0);
    readEnvelope(ps, PP_TONE_ENV_T1TIME, iniparser_getstring(ini, "tone:envelope", "0,100 5250,30 10500,0"));

    ps->params[PP_NOIZ_ON] = (float) iniparser_getint(ini, "noise:on", 0);
    ps->params[PP_NOIZ_LEVEL] = (float) iniparser_getint(ini, "noise:level", 128);
    ps->params[PP_NOIZ_SLOPE] = (float) iniparser_getint(ini, "noise:slope", 0);
    ps->params[PP_NOIZ_FIXEDSEQ] = (float) iniparser_getint(ini, "noise:fixedseq", 0);
    readEnvelope(ps, PP_NOIZ_ENV_T1TIME, iniparser_getstring(ini, "noise:envelope", "0,100 894,45 2756,18 5289,8 20113,0"));

    ps->params[PP_OTON_ON] = (float) iniparser_getint(ini, "overtones:on", 0);
    ps->params[PP_OTON_LEVEL] = (float) iniparser_getint(ini, "overtones:level", 128);
    ps->params[PP_OTON_F1] = (float) iniparser_getdouble(ini, "overtones:f1", 100.0);
    ps->params[PP_OTON_WAVE1] = (float) iniparser_getint(ini, "overtones:wave1", 0);
    ps->params[PP_OTON_TRACK1] = (float) iniparser_getint(ini, "overtones:track1", 0);
    ps->params[PP_OTON_F2] = (float) iniparser_getdouble(ini, "overtones:f2", 60.0);
    ps->params[PP_OTON_WAVE2] = (float) iniparser_getint(ini, "overtones:wave2", 1);
    ps->params[PP_OTON_TRACK2] = (float) iniparser_getint(ini, "overtones:track2", 0);
    ps->params[PP_OTON_METHOD] = (float) iniparser_getint(ini, "overtones:method", 0);
    ps->params[PP_OTON_PARAM] = (float) iniparser_getint(ini, "overtones:param", 0);
    ps->params[PP_OTON_FILTER] = (float) iniparser_getint(ini, "overtones:filter", 0);
    readEnvelope(ps, PP_OTON1_ENV_T1TIME, iniparser_getstring(ini, "overtones:envelope1", "0,100 1341,37 5215,10 20784,0"));
    readEnvelope(ps, PP_OTON2_ENV_T1TIME, iniparser_getstring(ini, "overtones:envelope2", "0,100 1341,37 5215,10 20784,0"));

    ps->params[PP_NBA1_ON] = (float) iniparser_getint(ini, "noiseband:on", 0);
    ps->params[PP_NBA1_LEVEL] = (float) iniparser_getint(ini, "noiseband:level", 128);
    ps->params[PP_NBA1_F] = (float) iniparser_getdouble(ini, "noiseband:f", 100.0);
    ps->params[PP_NBA1_DF] = (float) iniparser_getint(ini, "noiseband:df", 0);
    readEnvelope(ps, PP_NBA1_ENV_T1TIME, iniparser_getstring(ini, "noiseband:envelope", "0,100 1639,38 4619,20 10802,10 21082,0"));

    ps->params[PP_NBA2_ON] = (float) iniparser_getint(ini, "noiseband2:on", 0);
    ps->params[PP_NBA2_LEVEL] = (float) iniparser_getint(ini, "noiseband2:level", 128);
    ps->params[PP_NBA2_F] = (float) iniparser_getdouble(ini, "noiseband2:f", 100.0);
    ps->params[PP_NBA2_DF] = (float) iniparser_getint(ini, "noiseband2:df", 0);
    readEnvelope(ps, PP_NBA2_ENV_T1TIME, iniparser_getstring(ini, "noiseband2:envelope", "0,100 1639,38 4619,20 10802,10 21082,0"));

    ps->params[PP_DIST_ON] = (float) iniparser_getint(ini, "dist:on", 0);
    ps->params[PP_DIST_CLIPPING] = (float) iniparser_getint(ini, "dist:clipping", 128);
    ps->params[PP_DIST_BITS] = (float) iniparser_getint(ini, "dist:bits", 0);
    ps->params[PP_DIST_RATE] = (float) iniparser_getint(ini, "dist:rate", 0);

    iniparser_freedict(ini);

    return ps;
}
