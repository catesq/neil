#include "Ports.h"
#include <cmath>

#include "lv2/parameters/parameters.h"
#include "lv2/time/time.h"
#include "lv2/port-groups/port-groups.h"
#include "lv2_utils.h"

Port::Port(const LilvPort *lilvPort,
           const LilvPlugin* lilvPlugin,
           SharedCache* cache,
           PortType type,
           PortFlow flow,
           PortCounter& counter )
    : flow(flow),
      type(type),
      index(counter.total)
{
    properties = get_port_properties(cache, lilvPlugin, lilvPort);
    designation = get_port_designation(cache, lilvPlugin, lilvPort);

    name = as_string(lilv_port_get_name(lilvPlugin, lilvPort), true);
    symbol = as_string(lilv_port_get_symbol(lilvPlugin, lilvPort));
}



// some data used by Control and Parameter ports initialised here
ValuePort::ValuePort(const LilvPort *lilvPort,
           const LilvPlugin* lilvPlugin,
           SharedCache* cache,
           PortType type,
           PortFlow flow,
           PortCounter& counter)
    : Port(lilvPort, lilvPlugin, cache, type, flow, counter)
{
    LilvNode *default_val_node,
             *min_val_node,
             *max_val_node;

    lilv_port_get_range(lilvPlugin,
                        lilvPort,
                        &default_val_node,
                        &min_val_node,
                        &max_val_node);

    defaultValue = default_val_node != NULL ? as_float(default_val_node) : 0.f;
    minimumValue = min_val_node != NULL     ? as_float(min_val_node)     : 0.f;
    maximumValue = max_val_node != NULL     ? as_float(max_val_node)     : 1.f;

    if(maximumValue < minimumValue) {
        maximumValue = minimumValue + 1.0f;
    }

    if(defaultValue < minimumValue || defaultValue > maximumValue) {
        defaultValue = (maximumValue - minimumValue) / 2.0f;
    }
}


// basic copy/move constructors
ValuePort::ValuePort(const ValuePort& valuePort)
    : Port(valuePort),
      value(valuePort.value),
      minimumValue(valuePort.minimumValue),
      maximumValue(valuePort.maximumValue),
      defaultValue(valuePort.defaultValue)
{
}


ControlPort::ControlPort(const ControlPort& controlPort)
    : ValuePort(controlPort),
      controlIndex(controlPort.controlIndex)
{
}

ParamPort::ParamPort(ParamPort&& paramPort)
    : ValuePort(paramPort),
      paramIndex(paramPort.paramIndex),
      zzubValOffset(paramPort.zzubValOffset),
      zzubValSize(paramPort.zzubValSize)
{
}


ParamPort::ParamPort(const ParamPort& paramPort)
    : ValuePort(paramPort),
      paramIndex(paramPort.paramIndex),
      zzubValOffset(paramPort.zzubValOffset),
      zzubValSize(paramPort.zzubValSize)
{
    // each copy of the ParamPort needs it own copy of the zzub param
    // the zzub::info kinda holds a reference copy of the ParamPort/zzub::param
    // and each instance of the zzub::plugin has it's own copy of both
    // other plugins can use a central reference copy and not use local copies
    // but zzub::plugin::load(from save file) has to overwrite the param::value_default field
    // so each parameter is re-initiliased correctly.
    memcpy(&zzubParam, &paramPort.zzubParam, sizeof(zzub::parameter));

    for(auto& scalePoint: paramPort.scalePoints)
        scalePoints.push_back(scalePoint);
}


ControlPort::ControlPort(const LilvPort *lilvPort,
                         const LilvPlugin* lilvPlugin,
                         SharedCache* cache,
                         PortType type,
                         PortFlow flow,
                         PortCounter& counter )
    : ValuePort(lilvPort, lilvPlugin, cache, type, flow, counter),
      controlIndex(counter.control)
{
    counter.control++;
}


ParamPort::ParamPort(const LilvPort *lilvPort,
                     const LilvPlugin* lilvPlugin,
                     SharedCache* cache,
                     PortType type,
                     PortFlow flow,
                     PortCounter& counter )
    : ValuePort(lilvPort, lilvPlugin, cache, type, flow, counter),
      paramIndex(counter.param)
{
    zzubParam.flags = zzub::parameter_flag_state;


    LilvScalePoints *lilv_scale_points = lilv_port_get_scale_points(lilvPlugin, lilvPort);
    unsigned scale_points_size = scale_size(lilv_scale_points);

    if (LV2_IS_PORT_TOGGLED(properties) || LV2_IS_PORT_TRIGGER(properties)) {
        zzubParam.set_switch();
        zzubParam.value_default = zzub::switch_value_off;
    } else if(LV2_IS_PORT_ENUMERATION(properties)) {
        (scale_points_size <= 128) ?  zzubParam.set_byte() : zzubParam.set_word();
        zzubParam.value_default = (int) defaultValue;
        zzubParam.value_max = scale_points_size;
    } else if (LV2_IS_PORT_INTEGER(properties)) {
        (maximumValue - minimumValue <= 128) ? zzubParam.set_byte() : zzubParam.set_word();
        zzubParam.value_default = (int) defaultValue;
        zzubParam.value_min = 0;
        zzubParam.value_max = std::min((int)(maximumValue - minimumValue), 32768);
    } else {
        zzubParam.set_word();
        zzubParam.value_min = 0;
        zzubParam.value_max = 32768;
        zzubParam.value_default = lilv_to_zzub_value(defaultValue);
    }

    zzubParam.name        = name.c_str();
    zzubParam.description = zzubParam.name;
    zzubValSize           = zzubParam.get_bytesize();
    zzubValOffset         = counter.data;
    counter.data         += zzubValSize;

    if(lilv_scale_points != NULL ) {
        LILV_FOREACH(scale_points, spIter,lilv_scale_points) {
            const LilvScalePoint *lilvScalePoint = lilv_scale_points_get(lilv_scale_points, spIter);
            scalePoints.push_back(ScalePoint(lilvScalePoint));
            if(verbose) {
                auto last_item = scalePoints.size()-1;
                printf("\nScale point %f, %s", scalePoints[last_item].value, scalePoints[last_item].label.c_str());
            }
        }

        lilv_scale_points_free(lilv_scale_points);
    }

    counter.param++;
}

//void ControlPort::build_control_port() {
//    LilvNode *default_val_node, *min_val_node, *max_val_node;
//    lilv_port_get_range(&default_val_node, &min_val_node, &max_val_node);

//    defaultVal = default_val_node != NULL ? as_float(default_val_node) : 0.f;
//    minVal     = min_val_node != NULL     ? as_float(min_val_node) : 0.f;
//    maxVal     = min_val_node != NULL     ? as_float(max_val_node) : 0.f;

////    if(verbose) { printf("\nbuild port %s: ", name.c_str()); }

//    if(defaultVal < minVal || defaultVal > maxVal) {
//        defaultVal = (maxVal - minVal) / 2.0f;
//    }
//}

//void ParamPort::build_port_info() {
//    // the min, max and default values are collected in the control port.
////    ControlPort::build_port_info();

//    zzubParam.flags = zzub::parameter_flag_state;



//    LilvScalePoints *lilv_scale_points = lilv_port_get_scale_points(info->lilvPlugin, lilvPort);
//    unsigned scale_points_size = scale_size(lilv_scale_points);

//    if (LV2_IS_PORT_TOGGLED(properties) || LV2_IS_PORT_TRIGGER(properties)) {
////        if(verbose) { printf(" As switch param "); }
//        zzubParam.set_switch();
//        zzubParam.value_default = zzub::switch_value_off;
//    } else if(LV2_IS_PORT_ENUMERATION(properties)) {
////        if(verbose) { printf(" As options list "); }
//        (scale_points_size <= 128) ?  zzubParam.set_byte() : zzubParam.set_word();
//        zzubParam.value_default = (int) defaultVal;
//        zzubParam.value_max = scale_points_size;
//    } else if (LV2_IS_PORT_INTEGER(properties)) {
////        if(verbose) { printf(" as integer val "); }
//        (maxVal - minVal <= 128) ? zzubParam.set_byte() : zzubParam.set_word();
//        zzubParam.value_default = (int) defaultVal;
//        zzubParam.value_min = 0;
//        zzubParam.value_max = std::min((int)(maxVal - minVal), 32768);
//    } else {
////        if(verbose) { printf(" as word len slider "); }
//        //TODO check if logarithm, check for other properties
//        zzubParam.set_word();
//        zzubParam.value_min = 0;
//        zzubParam.value_max = 32768;
//        zzubParam.value_default = lilv_to_zzub_value(defaultVal);
//    }

//    zzubDataSize = zzubParam.get_bytesize();
//    zzubParam.name = name.c_str();
//    zzubParam.description = zzubParam.name;

//    if(lilv_scale_points != NULL ) {
//        LILV_FOREACH(scale_points, spIter,lilv_scale_points) {
//            const LilvScalePoint *lilvScalePoint = lilv_scale_points_get(lilv_scale_points, spIter);
//            scalePoints.push_back(new ScalePoint(lilvScalePoint));
//            if(verbose) { printf("\nScale point %f, %s", scalePoints[scalePoints.size()-1]->value, scalePoints[scalePoints.size()-1]->label.c_str()); }
//        }

//        lilv_scale_points_free(lilv_scale_points);
//    }
//}


//-----------------------------------------------------------------------------------


uint32_t get_port_properties(const SharedCache* cache, const LilvPlugin *lilvPlugin, const LilvPort *lilvPort) {
    uint32_t properties = 0;
    
    if (lilv_port_has_property(lilvPlugin, lilvPort, cache->nodes.pprop_optional))
        properties |= LV2_PORT_OPTIONAL;

    if (lilv_port_has_property(lilvPlugin, lilvPort, cache->nodes.pprop_enumeration))
        properties |= LV2_PORT_ENUMERATION;

    if (lilv_port_has_property(lilvPlugin, lilvPort, cache->nodes.pprop_integer))
        properties |= LV2_PORT_INTEGER;

    if (lilv_port_has_property(lilvPlugin, lilvPort, cache->nodes.pprop_sampleRate))
        properties |= LV2_PORT_SAMPLE_RATE;

    if (lilv_port_has_property(lilvPlugin, lilvPort, cache->nodes.pprop_toggled))
        properties |= LV2_PORT_TOGGLED;

    if (lilv_port_has_property(lilvPlugin, lilvPort, cache->nodes.pprop_artifacts) || lilv_port_has_property(lilvPlugin, lilvPort, cache->nodes.oldPropArtifacts))
        properties |= LV2_PORT_CAUSES_ARTIFACTS;

    if (lilv_port_has_property(lilvPlugin, lilvPort, cache->nodes.pprop_continuousCV) || lilv_port_has_property(lilvPlugin, lilvPort, cache->nodes.oldPropContinuousCV))
        properties |= LV2_PORT_CONTINUOUS_CV;

    if (lilv_port_has_property(lilvPlugin, lilvPort, cache->nodes.pprop_discreteCV) || lilv_port_has_property(lilvPlugin, lilvPort, cache->nodes.oldPropDiscreteCV))
        properties |= LV2_PORT_DISCRETE_CV;

    if (lilv_port_has_property(lilvPlugin, lilvPort, cache->nodes.pprop_expensive) || lilv_port_has_property(lilvPlugin, lilvPort, cache->nodes.oldPropExpensive))
        properties |= LV2_PORT_EXPENSIVE;

    if (lilv_port_has_property(lilvPlugin, lilvPort, cache->nodes.pprop_strictBounds) || lilv_port_has_property(lilvPlugin, lilvPort, cache->nodes.oldPropStrictBounds))
        properties |= LV2_PORT_STRICT_BOUNDS;

    if (lilv_port_has_property(lilvPlugin, lilvPort, cache->nodes.pprop_logarithmic) || lilv_port_has_property(lilvPlugin, lilvPort, cache->nodes.oldPropLogarithmic))
        properties |= LV2_PORT_LOGARITHMIC;

    if (lilv_port_has_property(lilvPlugin, lilvPort, cache->nodes.pprop_notAutomatic) || lilv_port_has_property(lilvPlugin, lilvPort, cache->nodes.oldPropNotAutomatic))
        properties |= LV2_PORT_NOT_AUTOMATIC;

    if (lilv_port_has_property(lilvPlugin, lilvPort, cache->nodes.pprop_notOnGUI) || lilv_port_has_property(lilvPlugin, lilvPort, cache->nodes.oldPropNotOnGUI))
        properties |= LV2_PORT_NOT_ON_GUI;

    if (lilv_port_has_property(lilvPlugin, lilvPort, cache->nodes.pprop_trigger) || lilv_port_has_property(lilvPlugin, lilvPort, cache->nodes.oldPropTrigger))
        properties |= LV2_PORT_TRIGGER;

    if (lilv_port_has_property(lilvPlugin, lilvPort, cache->nodes.pprop_nonAutomable))
        properties |= LV2_PORT_NON_AUTOMABLE;

    return properties;
}


//-----------------------------------------------------------------------------------


uint32_t get_port_designation(const SharedCache* cache, const LilvPlugin *lilvPlugin, const LilvPort *lilvPort) {
    uint32_t designation = 0;

    if (lilv_port_has_property(lilvPlugin, lilvPort, cache->nodes.reportsLatency))
        designation |= LV2_PORT_DESIGNATION_LATENCY;

    LilvNode* const designationNode = lilv_port_get(lilvPlugin, lilvPort, cache->nodes.designation);

    if (designationNode) {
        std::string designationStr = as_string(designationNode);

        if (designationStr.length() > 0) {
            if (std::strcmp(designationStr.c_str(), LV2_CORE__control) == 0)
                designation |= LV2_PORT_DESIGNATION_CONTROL;

            else if (std::strcmp(designationStr.c_str(), LV2_CORE__freeWheeling) == 0)
                designation |= LV2_PORT_DESIGNATION_FREEWHEELING;

            else if (std::strcmp(designationStr.c_str(), LV2_CORE__latency) == 0)
                designation |= LV2_PORT_DESIGNATION_LATENCY;

            else if (std::strcmp(designationStr.c_str(), LV2_PARAMETERS__sampleRate) == 0)
                designation |= LV2_PORT_DESIGNATION_SAMPLE_RATE;

            else if (std::strcmp(designationStr.c_str(), LV2_TIME__bar) == 0)
                designation |= LV2_PORT_DESIGNATION_TIME_BAR;

            else if (std::strcmp(designationStr.c_str(), LV2_TIME__barBeat) == 0)
                designation |= LV2_PORT_DESIGNATION_TIME_BAR_BEAT;

            else if (std::strcmp(designationStr.c_str(), LV2_TIME__beat) == 0)
                designation |= LV2_PORT_DESIGNATION_TIME_BEAT;

            else if (std::strcmp(designationStr.c_str(), LV2_TIME__beatUnit) == 0)
                designation |= LV2_PORT_DESIGNATION_TIME_BEAT_UNIT;

            else if (std::strcmp(designationStr.c_str(), LV2_TIME__beatsPerBar) == 0)
                designation |= LV2_PORT_DESIGNATION_TIME_BEATS_PER_BAR;

            else if (std::strcmp(designationStr.c_str(), LV2_TIME__beatsPerMinute) == 0)
                designation |= LV2_PORT_DESIGNATION_TIME_BEATS_PER_MINUTE;

            else if (std::strcmp(designationStr.c_str(), LV2_TIME__frame) == 0)
                designation |= LV2_PORT_DESIGNATION_TIME_FRAME;

            else if (std::strcmp(designationStr.c_str(), LV2_TIME__framesPerSecond) == 0)
                designation |= LV2_PORT_DESIGNATION_TIME_FRAMES_PER_SECOND;

            else if (std::strcmp(designationStr.c_str(), LV2_TIME__speed) == 0)
                designation |= LV2_PORT_DESIGNATION_TIME_SPEED;

            else if (std::strcmp(designationStr.c_str(), LV2_KXSTUDIO_PROPERTIES__TimePositionTicksPerBeat) == 0)
                designation |= LV2_PORT_DESIGNATION_TIME_TICKS_PER_BEAT;

            else if (std::strncmp(designationStr.c_str(), LV2_PARAMETERS_PREFIX, std::strlen(LV2_PARAMETERS_PREFIX)) != 0 &&
                     std::strncmp(designationStr.c_str(), LV2_PORT_GROUPS_PREFIX, std::strlen(LV2_PORT_GROUPS_PREFIX)) != 0) {
                fprintf(stderr, "lv2_port_designation(\"%s, %s\") - got unknown port designation '%s'",
                        as_string(lilv_plugin_get_name(lilvPlugin), true).c_str(),
                        as_string(lilv_port_get_name(lilvPlugin, lilvPort), true).c_str(),
                        designationStr.c_str());
            }
        }

        lilv_node_free(designationNode);
    }

    return designation;
}


//-----------------------------------------------------------------------------------

// u_int64_t get_plugin_type(const PluginWorld *world, const LilvPlugin *lilvPlugin) {
//     LilvNodes *typeNodes = lilv_plugin_get_value(lilvPlugin, world->rdf_type);

//     u_int64_t type = 0;

//     if (nodes_size(typeNodes) > 0) {
//         if (lilv_nodes_contains(typeNodes, world->class_allpass))
//             type |= LV2_PLUGIN_ALLPASS;
//         if (lilv_nodes_contains(typeNodes, world->class_amplifier))
//             type |= LV2_PLUGIN_AMPLIFIER;
//         if (lilv_nodes_contains(typeNodes, world->class_analyzer))
//             type |= LV2_PLUGIN_ANALYSER;
//         if (lilv_nodes_contains(typeNodes, world->class_bandpass))
//             type |= LV2_PLUGIN_BANDPASS;
//         if (lilv_nodes_contains(typeNodes, world->class_chorus))
//             type |= LV2_PLUGIN_CHORUS;
//         if (lilv_nodes_contains(typeNodes, world->class_comb))
//             type |= LV2_PLUGIN_COMB;
//         if (lilv_nodes_contains(typeNodes, world->class_compressor))
//             type |= LV2_PLUGIN_COMPRESSOR;
//         if (lilv_nodes_contains(typeNodes, world->class_constant))
//             type |= LV2_PLUGIN_CONSTANT;
//         if (lilv_nodes_contains(typeNodes, world->class_converter))
//             type |= LV2_PLUGIN_CONVERTER;
//         if (lilv_nodes_contains(typeNodes, world->class_delay))
//             type |= LV2_PLUGIN_DELAY;
//         if (lilv_nodes_contains(typeNodes, world->class_distortion))
//             type |= LV2_PLUGIN_DISTORTION;
//         if (lilv_nodes_contains(typeNodes, world->class_dynamics))
//             type |= LV2_PLUGIN_DYNAMICS;
//         if (lilv_nodes_contains(typeNodes, world->class_eq))
//             type |= LV2_PLUGIN_EQ;
//         if (lilv_nodes_contains(typeNodes, world->class_envelope))
//             type |= LV2_PLUGIN_ENVELOPE;
//         if (lilv_nodes_contains(typeNodes, world->class_expander))
//             type |= LV2_PLUGIN_EXPANDER;
//         if (lilv_nodes_contains(typeNodes, world->class_filter))
//             type |= LV2_PLUGIN_FILTER;
//         if (lilv_nodes_contains(typeNodes, world->class_flanger))
//             type |= LV2_PLUGIN_FLANGER;
//         if (lilv_nodes_contains(typeNodes, world->class_function))
//             type |= LV2_PLUGIN_FUNCTION;
//         if (lilv_nodes_contains(typeNodes, world->class_gate))
//             type |= LV2_PLUGIN_GATE;
//         if (lilv_nodes_contains(typeNodes, world->class_generator))
//             type |= LV2_PLUGIN_GENERATOR;
//         if (lilv_nodes_contains(typeNodes, world->class_highpass))
//             type |= LV2_PLUGIN_HIGHPASS;
//         if (lilv_nodes_contains(typeNodes, world->class_instrument))
//             type |= LV2_PLUGIN_INSTRUMENT;
//         if (lilv_nodes_contains(typeNodes, world->class_limiter))
//             type |= LV2_PLUGIN_LIMITER;
//         if (lilv_nodes_contains(typeNodes, world->class_lowpass))
//             type |= LV2_PLUGIN_LOWPASS;
//         if (lilv_nodes_contains(typeNodes, world->class_mixer))
//             type |= LV2_PLUGIN_MIXER;
//         if (lilv_nodes_contains(typeNodes, world->class_modulator))
//             type |= LV2_PLUGIN_MODULATOR;
//         if (lilv_nodes_contains(typeNodes, world->class_multiEQ))
//             type |= LV2_PLUGIN_MULTI_EQ;
//         if (lilv_nodes_contains(typeNodes, world->class_oscillator))
//             type |= LV2_PLUGIN_OSCILLATOR;
//         if (lilv_nodes_contains(typeNodes, world->class_paraEQ))
//             type |= LV2_PLUGIN_PARA_EQ;
//         if (lilv_nodes_contains(typeNodes, world->class_phaser))
//             type |= LV2_PLUGIN_PHASER;
//         if (lilv_nodes_contains(typeNodes, world->class_pitch))
//             type |= LV2_PLUGIN_PITCH;
//         if (lilv_nodes_contains(typeNodes, world->class_reverb))
//             type |= LV2_PLUGIN_REVERB;
//         if (lilv_nodes_contains(typeNodes, world->class_simulator))
//             type |= LV2_PLUGIN_SIMULATOR;
//         if (lilv_nodes_contains(typeNodes, world->class_spatial))
//             type |= LV2_PLUGIN_SPATIAL;
//         if (lilv_nodes_contains(typeNodes, world->class_spectral))
//             type |= LV2_PLUGIN_SPECTRAL;
//         if (lilv_nodes_contains(typeNodes, world->class_utility))
//             type |= LV2_PLUGIN_UTILITY;
//         if (lilv_nodes_contains(typeNodes, world->class_waveshaper))
//             type |= LV2_PLUGIN_WAVESHAPER;
//     }

// 	if(typeNodes != nullptr) {
// 		lilv_nodes_free(typeNodes);
// 	}

//     return type;
// }

int ParamPort::lilv_to_zzub_value(float lilv_val) {
//        if(strcmp(zzubParam.name, "pan_one") == 0 || strcmp(zzubParam.name, "kit_num") == 0 || strcmp(zzubParam.name, "base_note") == 0) {
//            printf("%s zzubmax %i min %f max %f from curr %f to zzub %i",
//                   zzubParam.name, zzubParam.value_max, minimumValue, maximumValue, lilv_val, (int)(((lilv_val - minimumValue) / (maximumValue - minimumValue)) * zzubParam.value_max));
//        }

    switch(zzubParam.type) {
    case zzub::parameter_type_note:
        return (int) lilv_val;

    case zzub::parameter_type_switch:
        return lilv_val == 0.f ? 1 : 0;

    case zzub::parameter_type_word:
    case zzub::parameter_type_byte:
        return zzubParam.value_min +
               (int)(
                 ((lilv_val - minimumValue) / (maximumValue - minimumValue)) * (zzubParam.value_max - zzubParam.value_min)
               );
    }
}

float ParamPort::zzub_to_lilv_value(int zzub_val) {
    switch(zzubParam.type) {
    case zzub::parameter_type_word:
    case zzub::parameter_type_byte:
        return minimumValue + ((zzub_val - zzubParam.value_min) /
               (float) (zzubParam.value_max - zzubParam.value_min)) * (maximumValue - minimumValue);

    case zzub::parameter_type_note:
        return (float) zzub_val;

    case zzub::parameter_type_switch:
        return zzub_val == zzub::switch_value_off ? 0.f : 1.f;
    }
}

int ParamPort::getData(uint8_t *globals){
    switch(zzubParam.type) {
    case zzub::parameter_type_word:
        return *((unsigned short*)(globals + zzubValOffset));

    case zzub::parameter_type_byte:
    case zzub::parameter_type_note:
    case zzub::parameter_type_switch:
        return *(globals + zzubValOffset);
    }
}

void ParamPort::putData(uint8_t *globals, int value) {
    uint8_t* dest = &globals[zzubValOffset];
    switch(zzubParam.type) {

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

const char* ParamPort::describeValue(const int value, char *text) {
    switch(zzubParam.type) {
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
        for(ScalePoint& scalePoint: scalePoints) {
            if(SIMILAR(lv2Val, scalePoint.value)) {
                sprintf(text, "%s", scalePoint.label.c_str());
                return text;
            }
        }
    }

    sprintf(text, "%f", lv2Val);
    return text;
}

