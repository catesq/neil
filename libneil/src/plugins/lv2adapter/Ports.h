#pragma once

#include "lilv/lilv.h"
#include "zzub/plugin.h"

#include "lv2_defines.h"
#include "PluginWorld.h"
#include "ext/lv2_evbuf.h"
#include "lv2_utils.h"


struct PluginAdapter;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

enum PortFlow : unsigned {
    Unknown = 0,
    Input   = 1,
    Output  = 2,
};

enum PortType : unsigned {
    BadPort = 0,
    Audio   = 2,
    Control = 4,
    Param   = 8,
    CV      = 16,
    Event   = 32,
    Midi    = 64
};

u_int32_t get_port_properties(const SharedAdapterCache *cache, const LilvPlugin *lilvPlugin, const LilvPort *lilvPort);
u_int32_t get_port_designation(const SharedAdapterCache *cache, const LilvPlugin *lilvPlugin, const LilvPort *lilvPort);


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

union BodgeEndian {
    uint16_t i;
    uint8_t c[2];
};



struct Port {
    Port(PortType type, PortFlow flow, uint32_t index);
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
    using Port::Port;

    float value = 0.f;
    float minimumValue;
    float maximumValue;
    float defaultValue;

    virtual void* data_pointer() override { return &value; }
};


struct ControlPort : ValuePort {
    uint32_t controlIndex;

    ControlPort(PortFlow flow, uint32_t index, uint32_t controlIndex);
};

struct ParamPort : ValuePort {
    ParamPort(PortFlow flow, uint32_t index, uint32_t paramIndex);

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


