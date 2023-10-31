#pragma once

#include "libzzub/host.h"

namespace zzub {

struct outstream;


struct host_info {
    int id;
    int version;
    void *host_ptr;  // host-specific data
};


struct lib {
    virtual void get_instrument_list(outstream *os) = 0;
};


struct midi_message {
    int device;
    unsigned long message;
    unsigned long timestamp;
};



struct master_info {
    int beats_per_minute;
    int ticks_per_beat;
    int samples_per_second;
    int samples_per_tick;
    int tick_position;
    float ticks_per_second;
    float samples_per_tick_frac;  // zzub extension
};


struct scopelock {
    scopelock(host *h) {
        this->h = h;
        h->lock();
    }
    ~scopelock() {
        h->unlock();
    }

    host *h;
};

#define SIGNAL_TRESHOLD (0.0000158489f)

inline bool buffer_has_signals(const float *buffer, int ns) {
    while (ns--) {
        if ((*buffer > SIGNAL_TRESHOLD) || (*buffer < -SIGNAL_TRESHOLD)) {
            return true;
        }
        buffer++;
    }
    return false;
}

#define ZZUB_PLUGIN_LOCK zzub::scopelock _sl(this->_host);

}