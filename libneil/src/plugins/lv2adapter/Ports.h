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
    Control = 8,
    CV      = 16,
    Event   = 32,
    Midi    = 64
};

//ParamPort *make_param_port(const PluginWorld *world, ParamPort *paramPort, PluginInfo *info);

// u_int64_t get_plugin_type(const PluginWorld *world, const LilvPlugin *lilvPlugin);
u_int32_t get_port_properties(const PluginWorld *world, const LilvPlugin *lilvPlugin, const LilvPort *lilvPort);
u_int32_t get_port_designation(const PluginWorld *world, const LilvPlugin *lilvPlugin, const LilvPort *lilvPort);


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


struct ScalePoint {
    float value;
    std::string label;

    ScalePoint(const LilvScalePoint *scalePoint) {
        label = lilv::as_string(lilv_scale_point_get_label(scalePoint));
        value = lilv::as_float(lilv_scale_point_get_value(scalePoint));
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
    // Evbuf_Type apiType;
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


struct ParamPort : Port {
    zzub::parameter *zzubParam;
    std::vector<ScalePoint *> scalePoints{};

    unsigned paramIndex;
    unsigned byteOffset;
    unsigned byteSize;

    float minVal;
    float maxVal;
    float defaultVal;

    ParamPort(
        PluginInfo *info,
        const LilvPort *lilvPort,
        PortFlow flow,
        uint32_t index,
        unsigned paramIndex,
        unsigned byteOffset
    );

    ~ParamPort() {

    }

    inline int lilv_to_zzub_value(float val) {
        if(strcmp(zzubParam->name, "pan_one") == 0 || strcmp(zzubParam->name, "kit_num") == 0 || strcmp(zzubParam->name, "base_note") == 0) {
            printf("%s zzubmax %i min %f max %f from curr %f to zzub %i",
                   zzubParam->name, zzubParam->value_max, minVal, maxVal, val, (int)(((val - minVal) / (maxVal - minVal)) * zzubParam->value_max));
        }

        switch(zzubParam->type) {
        case zzub_parameter_type_note:
            return (int) val;
            
        case zzub_parameter_type_switch:
            return val == 0.f ? 1 : 0;

        case zzub_parameter_type_word:
        case zzub_parameter_type_byte:
            return zzubParam->value_min + (int)(((val - minVal) / (maxVal - minVal)) * (zzubParam->value_max - zzubParam->value_min));
        }
    }

    inline float zzub_to_lilv_value(int val) {
        // if(strcmp(zzubParam->name, "pan_one") == 0 || strcmp(zzubParam->name, "kit_num") == 0 || strcmp(zzubParam->name, "base_note") == 0) {
        //     printf("%s zzubmax %i min %f max %f curr %i lv2curr %f",
        //            zzubParam->name, zzubParam->value_max, minVal, maxVal, val, minVal + (val / (float) zzubParam->value_max) * (maxVal - minVal));
        // }

        switch(zzubParam->type) {
        case zzub_parameter_type_word:
        case zzub_parameter_type_byte:
            return minVal + ((val - zzubParam->value_min) / (float) (zzubParam->value_max - zzubParam->value_min)) * (maxVal - minVal);
        case zzub_parameter_type_note:
            return (float) val;
        case zzub_parameter_type_switch:
            return val == zzub::switch_value_off ? 0.f : 1.f;
        }
    }

    int getData(uint8_t *globals){
        switch(zzubParam->type) {
        case zzub_parameter_type_word:
            return *((unsigned short*)(globals + byteOffset));
        case zzub_parameter_type_byte:
        case zzub_parameter_type_note:
            return *(globals + byteOffset);
        }
    }

    void putData(uint8_t *globals, int value) {
        printf("putdata %d %d\n", zzubParam->type, byteOffset);
        uint8_t* dest = &globals[byteOffset];
        switch(zzubParam->type) {
        case zzub_parameter_type_word:{
            auto be = BodgeEndian{(uint16_t)value};
            printf("word to %d\n", byteOffset);
            *dest = be.c[0];
            *dest++ = be.c[1];
            break;
        }
        case zzub_parameter_type_byte:
        printf("bte to %d\n", byteOffset);
            *dest = (u_int8_t) value;
            break;
        case zzub_parameter_type_note:
        printf("note to %d\n", byteOffset);
            *dest = (u_int8_t) value;
            break;
        case zzub_parameter_type_switch:
        printf("bool to %d\n", byteOffset);
            *dest = (value == 0.f ? zzub::switch_value_off : zzub::switch_value_on);
            break;
        }
        // printf("dione put\n");
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
//            printf("describe note");
            return note_param_to_str(value, text);
        }

        if(LV2_IS_PORT_INTEGER(properties)) {
//            printf("describe integer");
            sprintf(text, "%i", value);
            return text;
        }

        float lv2Val = zzub_to_lilv_value(value);

        if (scalePoints.size() > 0) {
            for(ScalePoint* scalePoint: scalePoints) {
                if(SIMILAR(lv2Val, scalePoint->value)) {
//                    printf("describe scalepoint");
//                    return scalePoint->label.c_str();
                    sprintf(text, "%s", scalePoint->label.c_str());
                    return text;
                }
            }
        }

//        printf("describe float");
        sprintf(text, "%f", lv2Val);
//        printf(": ");
        return text;
    }
};




#endif // LV2_PORTS_HPP
