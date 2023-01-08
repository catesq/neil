#pragma once

#include "lilv/lilv.h"
#include "zzub/plugin.h"

#include "lv2_defines.h"
#include "PluginWorld.h"
#include "ext/lv2_evbuf.h"



struct lv2_adapter;

// when the ports are being built the builder function in PluginInfo needs to track the number of control ports, parameter ports and total number of ports
// the data counter is only used by ParamPorts and is the offset in bytes into the zzub::plugin::global_values
struct PortCounter {
    uint32_t portIndex = 0;
    uint32_t control = 0;
    uint32_t param = 0;
    uint32_t dataOffset = 0;
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

    Port(const Port& port);

    virtual ~Port() {}

    std::string name;
    std::string symbol;

    uint32_t    properties;
    uint32_t    designation;

    PortFlow    flow;
    PortType    type;
    uint32_t    index;

    virtual void* data_pointer() { return nullptr; }

    virtual unsigned get_zzub_flags() { return 0; }
};


struct AudioBufPort: Port {
    float* buf = nullptr;

    using Port::Port;

    virtual ~AudioBufPort() override {
        if(buf != nullptr){
            free(buf);
        }
    }


    virtual void* data_pointer() override { return buf; }

    virtual unsigned get_zzub_flags() override {
        if (type == PortType::CV) {
            return flow == PortFlow::Input ? zzub_plugin_flag_has_cv_input : zzub_plugin_flag_has_cv_output;
        } else if (type == PortType::Audio) {
            return flow == PortFlow::Input ? zzub_plugin_flag_has_audio_input : zzub_plugin_flag_has_audio_output;
        }

        return 0;
    }
};


struct EventBufPort : Port {
    LV2_Evbuf* eventBuf   = nullptr;

    using Port::Port;

    virtual ~EventBufPort() override {
        if(eventBuf != nullptr) {
            lv2_evbuf_free(eventBuf);
        }
    }

    virtual void* data_pointer() override {
        return lv2_evbuf_get_buffer(eventBuf);
    }

    virtual unsigned get_zzub_flags() override {
        return PortFlow::Input ? zzub_plugin_flag_has_event_input : zzub_plugin_flag_has_event_output;
    }
};


struct ValuePort: Port {
    float value = 0.f;
    float minimumValue;
    float maximumValue;
    float defaultValue;

    ValuePort(const ValuePort& vPort);

    ValuePort(const LilvPort *lilvPort,
              const LilvPlugin* lilvPlugin,
              SharedCache* cache,
              PortType type,
              PortFlow flow,
              PortCounter& counter);

    virtual void* data_pointer() override {
        return &value;
    }

    virtual unsigned get_zzub_flags() override {
        return PortFlow::Input ? zzub_plugin_flag_has_midi_input : zzub_plugin_flag_has_midi_output;
    }
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

    virtual unsigned get_zzub_flags() override {
        return zzub_plugin_flag_has_event_output;
    }
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

    virtual unsigned get_zzub_flags() override {
        return zzub_plugin_flag_has_event_input;
    }

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


