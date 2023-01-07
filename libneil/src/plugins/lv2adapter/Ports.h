#pragma once

#include "lilv/lilv.h"
#include "zzub/plugin.h"

#include "lv2_defines.h"
#include "PluginWorld.h"
#include "ext/lv2_evbuf.h"



struct PluginAdapter;

// when the ports are being built the builder function in PluginInfo needs to track the number of control ports, parameter ports and total number of ports
// the data counter is only used by ParamPorts and is the offset in bytes into the zzub::plugin::global_values
struct PortCounter {
    uint32_t total = 0;
    uint32_t control = 0;
    uint32_t param = 0;
    uint32_t data = 0;
};
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


u_int32_t get_port_properties(const SharedCache* cache, const LilvPlugin *lilvPlugin, const LilvPort *lilvPort);
u_int32_t get_port_designation(const SharedCache* cache, const LilvPlugin *lilvPlugin, const LilvPort *lilvPort);


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


struct ScalePoint {
    float value;
    std::string label;

    ScalePoint(const LilvScalePoint *scalePoint) {
        label = as_string(lilv_scale_point_get_label(scalePoint));
        value = as_float(lilv_scale_point_get_value(scalePoint));
    }
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// the midi track is two bytes in a 16 bit field and the order of the bytes matters.
// this is needed for little endian systems
union BodgeEndian {
    uint16_t i;
    uint8_t c[2];
};



struct Port {
    // the lilvLilvPort, LilvPlugin and SharedCache are needed to assign the properties, designation and other class members and are not stored
    Port(const LilvPort *lilvPort,
         const LilvPlugin* lilvPlugin,
         SharedCache* cache,
         PortType type,
         PortFlow flow,
         PortCounter& counter
    );

    virtual ~Port() {}

    std::string name;
    std::string symbol;

    uint32_t    properties;
    uint32_t    designation;

    PortFlow    flow;
    PortType    type;
    uint32_t    index;

    virtual void* data_pointer() { return nullptr; }
};


struct AudioBufPort: Port {
    using Port::Port;

    virtual ~AudioBufPort() override {
        if(buf != nullptr)
            free(buf);
    }

    float* buf = nullptr;
    virtual void* data_pointer() override { return buf; }
};


struct EventBufPort : Port {
    using Port::Port;

    virtual ~EventBufPort() override {
        if(eventBuf != nullptr)
            lv2_evbuf_free(eventBuf);
    }

    LV2_Evbuf* eventBuf   = nullptr;
    virtual void* data_pointer() override { return lv2_evbuf_get_buffer(eventBuf); }
};


struct ValuePort: Port {
    ValuePort(const LilvPort *lilvPort,
              const LilvPlugin* lilvPlugin,
              SharedCache* cache,
              PortType type,
              PortFlow flow,
              PortCounter& counter
    );

    ValuePort(const ValuePort& vPort);

    float value = 0.f;
    float minimumValue;
    float maximumValue;
    float defaultValue;

    virtual void* data_pointer() override { return &value; }
};


struct ControlPort : ValuePort {
    uint32_t controlIndex;

    ControlPort(const LilvPort *lilvPort,
                const LilvPlugin* lilvPlugin,
                SharedCache* cache,
                PortType type,
                PortFlow flow,
                PortCounter& counter
    );
    ControlPort(const ControlPort& controlPort);
};


struct ParamPort : ValuePort {
    ParamPort(const LilvPort *lilvPort,
              const LilvPlugin* lilvPlugin,
              SharedCache* cache,
              PortType type,
              PortFlow flow,
              PortCounter& counter
    );
    ParamPort(const ParamPort& paramPort);
    ParamPort(ParamPort&& paramPort);

    int         lilv_to_zzub_value(float lilv_val);
    float       zzub_to_lilv_value(int zzub_val);
    int         getData(uint8_t *globals);
    void        putData(uint8_t *globals, int value);
    const char* describeValue(const int value, char *text);

    uint32_t  paramIndex;
    uint32_t  zzubValOffset;
    uint32_t  zzubValSize;

    zzub::parameter         zzubParam;
    std::vector<ScalePoint> scalePoints{};
};


