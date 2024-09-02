
#include <zzub/zzub.h>
#include <zzub/signature.h>
#include <zzub/plugin.h>

#include "Utils.hpp"

const char *zzub_get_signature() {
    return ZZUB_SIGNATURE;
}

class DCBlock : public zzub::plugin {
    lanternfish::DcFilter dcblock;
public:
    DCBlock() { }
    virtual ~DCBlock() {}
    virtual void init(zzub::archive* pi) {}
    virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode) {
        if (mode == zzub::process_mode_write || mode == zzub::process_mode_no_io) {
          return false;
        } else if (mode == zzub::process_mode_read) {
          return true;
        }

        dcblock.process(pin[0], pout[0], pin[1], pout[1], numsamples);

        return true;
    }
};


struct DCBlockInfo : zzub::info {
    DCBlockInfo() {
        this->flags = zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output | zzub::plugin_flag_is_effect;
        this->min_tracks = 1;
        this->max_tracks = 1;
        this->name = "DCBlock";
        this->short_name = "DCBlock";
        this->author = "tnh";
        this->uri = "@libneil/effect/dcblock";
    }
    virtual zzub::plugin* create_plugin() const { return new DCBlock(); }
    virtual bool store_info(zzub::archive *data) const { return false; }
} DCBlockMachineInfo;


struct DCBlockPluginCollection : zzub::plugincollection {
    virtual void initialize(zzub::pluginfactory *factory) {
        factory->register_info(&DCBlockMachineInfo);
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


zzub::plugincollection *zzub_get_plugincollection() {
    return new DCBlockPluginCollection();
}
