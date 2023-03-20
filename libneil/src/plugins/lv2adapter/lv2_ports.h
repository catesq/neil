#pragma once

#include <memory>

#include "ext/lv2_evbuf.h"
#include "lilv/lilv.h"
#include "lv2_defines.h"
#include "lv2_lilv_world.h"
#include "lv2_utils.h"
#include "zzub/plugin.h"

struct lv2_adapter;

// when the ports are being built the builder function in PluginInfo needs to track the number of control ports, parameter ports and total number of ports
// the data counter is only used by param_ports and is the offset in bytes into the zzub::plugin::global_values
struct PortCounter {
    uint32_t portIndex = 0;
    uint32_t control = 0;
    uint32_t param = 0;
    uint32_t dataOffset = 0;
};
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

u_int32_t get_port_properties(const lv2_lilv_world* cache, const LilvPlugin* lilvPlugin, const LilvPort* lilvPort);
u_int32_t get_port_designation(const lv2_lilv_world* cache, const LilvPlugin* lilvPlugin, const LilvPort* lilvPort);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

struct ScalePoint {
    float value;
    std::string label;

    ScalePoint(const LilvScalePoint* scalePoint) {
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

struct lv2_port {
    // the LilvPort, LilvPlugin and lv2_lilv_world are needed to assign the properties, designation and other class members and are not stored
    lv2_port(const LilvPort* lilvPort,
             const LilvPlugin* lilvPlugin,
             lv2_lilv_world* cache,
             PortType type,
             PortFlow flow,
             PortCounter& counter);

    lv2_port(const lv2_port& port);

    virtual ~lv2_port() {}

    std::string name;
    std::string symbol;

    uint32_t properties;
    uint32_t designation;

    PortFlow flow;
    PortType type;
    uint32_t index;

    virtual void* data_pointer() { return nullptr; }

    virtual unsigned get_zzub_flags() { return 0; }
};

struct audio_buf_port : lv2_port {
   private:
    float* buffer = nullptr;

   public:
    audio_buf_port(const LilvPort* lilvPort,
                   const LilvPlugin* lilvPlugin,
                   lv2_lilv_world* cache,
                   PortType type,
                   PortFlow flow,
                   PortCounter& counter) : lv2_port(lilvPort, lilvPlugin, cache, type, flow, counter) {
    }

    audio_buf_port(const audio_buf_port& port) : lv2_port(port) {
    }

    void set_buffer(float* buf) {
        this->buffer = buf;
    }

    virtual ~audio_buf_port() override {
        if (buffer) {
            free(buffer);
        }
    }

    virtual void* data_pointer() override {
        return buffer;
    }

    float* get_buffer() {
        return buffer;
    }

    virtual unsigned get_zzub_flags() override {
        if (type == PortType::CV) {
            return flow == PortFlow::Input ? zzub_plugin_flag_has_cv_input : zzub_plugin_flag_has_cv_output;
        } else if (type == PortType::Audio) {
            return flow == PortFlow::Input ? zzub_plugin_flag_has_audio_input : zzub_plugin_flag_has_audio_output;
        }

        return 0;
    }
};

struct event_buf_port : lv2_port {
   private:
    LV2_Evbuf* event_buf = nullptr;

   public:
    event_buf_port(const LilvPort* lilvPort,
                   const LilvPlugin* lilvPlugin,
                   lv2_lilv_world* cache,
                   PortType type,
                   PortFlow flow,
                   PortCounter& counter) : lv2_port(lilvPort, lilvPlugin, cache, type, flow, counter){};

    event_buf_port(const event_buf_port& port) : lv2_port(port) {
    }

    virtual ~event_buf_port() override {
        printf("EVENT BUF DELETE %s\n", event_buf ? "YES" : "NO");
        if (event_buf) {
            lv2_evbuf_free(event_buf);
        }
    }

    virtual void* data_pointer() override {
        return lv2_evbuf_get_buffer(event_buf);
    }

    void set_buffer(LV2_Evbuf* buf) {
        event_buf = buf;
    }

    LV2_Evbuf* get_lv2_evbuf() {
        return event_buf;
    }

    virtual unsigned get_zzub_flags() override {
        return PortFlow::Input ? zzub_plugin_flag_has_event_input : zzub_plugin_flag_has_event_output;
    }
};

struct value_port : lv2_port {
    float value = 0.f;
    float minimumValue;
    float maximumValue;
    float defaultValue;

    value_port(const value_port& vPort);

    value_port(const LilvPort* lilvPort,
               const LilvPlugin* lilvPlugin,
               lv2_lilv_world* cache,
               PortType type,
               PortFlow flow,
               PortCounter& counter);

    virtual void* data_pointer() override {
        return &value;
    }

    void set_value(float value) {
        this->value = value;
    }

    float get_value() {
        return value;
    }

    virtual unsigned get_zzub_flags() override {
        return PortFlow::Input ? zzub_plugin_flag_has_midi_input : zzub_plugin_flag_has_midi_output;
    }
};

struct control_port : value_port {
    uint32_t controlIndex;

    control_port(const LilvPort* lilvPort,
                 const LilvPlugin* lilvPlugin,
                 lv2_lilv_world* cache,
                 PortType type,
                 PortFlow flow,
                 PortCounter& counter);

    control_port(const control_port& control_port);

    virtual unsigned get_zzub_flags() override {
        return zzub_plugin_flag_has_event_output;
    }
};

struct param_port : value_port {
    param_port(const LilvPort* lilvPort,
               const LilvPlugin* lilvPlugin,
               lv2_lilv_world* cache,
               PortType type,
               PortFlow flow,
               PortCounter& counter);
    param_port(param_port&& param_port);

    param_port(const param_port& param_port);

    virtual unsigned get_zzub_flags() override {
        return zzub_plugin_flag_has_event_input;
    }

    int lilv_to_zzub_value(float lilv_val);
    float zzub_to_lilv_value(int zzub_val);

    // zzub packs 8 bit and 16 bit integers into a memory block with no paddinng or alignments
    // get data uses zzubValOffset property to read the 8/16 bit integer which is then passed to zzub_to_lilv_value
    // before being stored as the float proerty value_port->value
    int getData(uint8_t* globals);

    // the reverse of getData
    void putData(uint8_t* globals, int value);

    //
    const char* describeValue(const int value, char* text);

    uint32_t paramIndex;
    uint32_t zzubValOffset;
    uint32_t zzubValSize;

    zzub::parameter zzubParam;
    std::vector<ScalePoint> scalePoints{};
};
