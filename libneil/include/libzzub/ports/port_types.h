
#pragma once


#include "zzub/plugin.h"



namespace zzub {


enum class port_event_type {
    port_event_type_value = 0,
};

union port_value {
    float f;
};

struct port_event {
    port_event_type event_type;
    uint32_t port_index;
    port_value value;
};


}; // namespace zzub