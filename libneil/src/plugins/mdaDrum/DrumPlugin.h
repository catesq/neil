#pragma once

#include <zzub/zzub.h>
#include <zzub/signature.h>
#include <zzub/plugin.h>
#include "math.h"
#include <vector>
#include <sys/types.h>

#include "DrumDefines.h"
#include "DrumVoice.h"
#include "DrumPresets.h"


struct DrumPluginInfo : zzub::info {
    DrumSets* drumSets;

    DrumPluginInfo(DrumSets* drumSets);

    virtual zzub::plugin* create_plugin() const;
    virtual bool store_info(zzub::archive *data) const { return false; }
};

#pragma pack(1)

struct DrumTvals {
    uint8_t note = 0;
    uint8_t volume = 255;
    uint16_t drumId = 0;
    uint16_t stretch = 0x100;

    void reset();
    float amplitude();
    float timestretch();
};

#pragma pack()



class DrumPlugin : public zzub::plugin {
private:
    std::vector<DrumVoice*> track_voices {};
    std::vector<DrumVoice*> active_voices {};
    std::vector<DrumVoice*> inactive_voices {};
    const DrumSets *drumSets {nullptr};
    DrumTvals tval[MAX_TRACK] {};
    DrumTvals tstate[MAX_TRACK] {};

    int track_count = 1;

    std::vector<DrumVoice*>::iterator find_inactive_voice(uint16_t drum_id);
    std::vector<DrumVoice*>::iterator find_inactive_voice(DrumVoice *voice);
    void set_voice(int index, uint16_t drum_id);
    void set_voice(int index, DrumVoice* voice);
    void delete_voices(std::vector<DrumVoice*>& voices);

public:
    // DrumPlugin();
    DrumPlugin(const DrumSets* drumSets);

    virtual ~DrumPlugin();

    virtual void destroy() {
        delete this;
    }

    virtual void event(unsigned int data);
    virtual void init(zzub::archive* pi);
    virtual void process_events();
    virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
    virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
    virtual const char * describe_value(int param, int value);
    virtual void set_track_count(int);
    void save(zzub::archive *arc);
    void load(zzub::archive *arc);


};



struct DrumPluginCollection : zzub::plugincollection {
    DrumSets* drumSets;

    virtual void initialize(zzub::pluginfactory *factory) {
		drumSets = new DrumSets ("/home/mylogin/.neil/mdaDrumLibrary", 256);
        factory->register_info(new DrumPluginInfo(drumSets));
    }

    ~DrumPluginCollection() {
        delete drumSets;
    }

    virtual const zzub::info *get_info(const char *uri, zzub::archive *data) {
        return 0;
    }

    virtual void destroy() {
        delete this;
    }

    virtual const char *get_uri() {
        return 0;
    }

    virtual void configure(const char *key, const char *value) {

    }
};

zzub::plugincollection *zzub_get_plugincollection();

const char *zzub_get_signature();
