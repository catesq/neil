#ifndef LV2_PORTS_HPP
#define LV2_PORTS_HPP

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
    None    = 0,
    Audio   = 4,
    Param   = 8,
    Control = 16,
    CV      = 32,
    Event   = 64,
    Midi    = 128
};

u_int32_t get_port_properties(const PluginWorld *world, const LilvPlugin *lilvPlugin, const LilvPort *lilvPort);
u_int32_t get_port_designation(const PluginWorld *world, const LilvPlugin *lilvPlugin, const LilvPort *lilvPort);


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

struct Port {
    const PluginInfo *info;
    const LilvPort *lilvPort;
    std::string name;
    std::string symbol;

    uint32_t properties;
    uint32_t designation;

    PortFlow flow;
    PortType type;
    uint32_t index;

    Port(PluginInfo *info, const LilvPort *lilvPort, PortFlow flow, PortType type, uint32_t index);
};

struct EventPort : Port {
    unsigned bufIndex;

    EventPort(PluginInfo *info,
              const LilvPort *lilvPort,
              PortFlow flow,
              PortType type,
              uint32_t index,
              unsigned bufIndex
              );
};



struct MidiPort : EventPort {
    MidiPort(
            PluginInfo *info,
            const LilvPort *lilvPort,
            PortFlow flow,
            uint32_t index,
            unsigned bufIndex
            );
};

struct BufPort  : Port {
    unsigned bufIndex;
    BufPort(
            PluginInfo *info,
            const LilvPort *lilvPort,
            PortFlow flow,
            PortType type,
            uint32_t index,
            unsigned bufIndex
            );
};


struct CvBufPort : BufPort {
    CvBufPort(
            PluginInfo *info,
            const LilvPort *lilvPort,
            PortFlow flow,
            uint32_t index,
            unsigned bufIndex
            );
};


struct AudioBufPort : BufPort {
    AudioBufPort(
            PluginInfo *info,
            const LilvPort *lilvPort,
            PortFlow flow,
            uint32_t index,
            unsigned bufIndex
            );
};

union BodgeEndian {
    uint16_t i;
    uint8_t c[2];
};

struct ControlPort : Port {
    uint32_t controlIndex;
    float defaultVal = 0.f;

    ControlPort(
            PluginInfo *info,
            const LilvPort *lilvPort,
            PortFlow flow,
            uint32_t index,
            uint32_t controlIndex
            );
};

struct ParamPort : Port {
    zzub::parameter *zzubParam;
    std::vector<ScalePoint *> scalePoints{};

    uint32_t byteOffset;
    uint32_t byteSize;
    uint32_t paramIndex;

    float minVal;
    float maxVal;
    float defaultVal;

    ParamPort(
            PluginInfo *info,
            const LilvPort *lilvPort,
            PortFlow flow,
            uint32_t index,
            uint32_t paramIndex,
            uint32_t byteOffset
            );

    inline int lilv_to_zzub_value(float val) {
        if(strcmp(zzubParam->name, "pan_one") == 0 || strcmp(zzubParam->name, "kit_num") == 0 || strcmp(zzubParam->name, "base_note") == 0) {
            printf("%s zzubmax %i min %f max %f from curr %f to zzub %i",
                   zzubParam->name, zzubParam->value_max, minVal, maxVal, val, (int)(((val - minVal) / (maxVal - minVal)) * zzubParam->value_max));
        }

        switch(zzubParam->type) {
        case zzub::parameter_type_note:
            return (int) val;

        case zzub::parameter_type_switch:
            return val == 0.f ? 1 : 0;

        case zzub::parameter_type_word:
        case zzub::parameter_type_byte:
            return zzubParam->value_min + (int)(((val - minVal) / (maxVal - minVal)) * (zzubParam->value_max - zzubParam->value_min));
        }
    }

    inline float zzub_to_lilv_value(int val) {
        switch(zzubParam->type) {
        case zzub::parameter_type_word:
        case zzub::parameter_type_byte:
            return minVal + ((val - zzubParam->value_min) / (float) (zzubParam->value_max - zzubParam->value_min)) * (maxVal - minVal);
        case zzub::parameter_type_note:
            return (float) val;
        case zzub::parameter_type_switch:
            return val == zzub::switch_value_off ? 0.f : 1.f;
        }
    }

    int getData(uint8_t *globals){
        switch(zzubParam->type) {
        case zzub::parameter_type_word:
            return *((unsigned short*)(globals + byteOffset));
        case zzub::parameter_type_byte:
        case zzub::parameter_type_note:
        case zzub::parameter_type_switch:
            return *(globals + byteOffset);
        }
    }

    void putData(uint8_t *globals, int value) {
        uint8_t* dest = &globals[byteOffset];
        switch(zzubParam->type) {

        case zzub::parameter_type_word: {
            auto be = BodgeEndian{(uint16_t)value};
            *dest = be.c[0];
            *dest++ = be.c[1];
            break;
        }

        case zzub::parameter_type_byte:
            *dest = (u_int8_t) value;
            break;

        case zzub::parameter_type_note:
            *dest = (u_int8_t) value;
            break;

        case zzub::parameter_type_switch:
            *dest = (value == 0.f ? zzub::switch_value_off : zzub::switch_value_on);
            break;
        }
    }

    const char *describeValue(const int value, char *text) {
        switch(zzubParam->type) {
        case zzub::parameter_type_switch:
            if(value == zzub::switch_value_on) {
                sprintf(text, "1");
            } else if(value == zzub::switch_value_off) {
                sprintf(text, "0");
            } else {
                sprintf(text, "-");
            }

            return text;

        case zzub::parameter_type_note:
            return note_param_to_str(value, text);

        default:
            break;
        }

        if(LV2_IS_PORT_INTEGER(properties)) {
            sprintf(text, "%i", value);
            return text;
        }

        float lv2Val = zzub_to_lilv_value(value);

        if (scalePoints.size() > 0) {
            for(ScalePoint* scalePoint: scalePoints) {
                if(SIMILAR(lv2Val, scalePoint->value)) {
                    sprintf(text, "%s", scalePoint->label.c_str());
                    return text;
                }
            }
        }

        sprintf(text, "%f", lv2Val);
        return text;
    }
};




#endif // LV2_PORTS_HPP
