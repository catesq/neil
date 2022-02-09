#include "PluginAdapter.h"
#include "Ports.h"
#include "PluginInfo.h"
#include <cmath>


Port::Port(
        PluginInfo *info, 
        const LilvPort *lilvPort, 
        PortFlow flow,
        PortType type,
        unsigned index
    ) : info(info),
        lilvPort(lilvPort),
        flow(flow),
        type(type),
        index(index) {

    properties = get_port_properties(info->world, info->lilvPlugin, lilvPort);
    designation = get_port_designation(info->world, info->lilvPlugin, lilvPort);

    name = lilv::as_string(lilv_port_get_name(info->lilvPlugin, lilvPort), true);
    symbol = lilv::as_string(lilv_port_get_symbol(info->lilvPlugin, lilvPort));
}

BufPort::BufPort(
        PluginInfo *info, 
        const LilvPort *lilvPort, 
        PortFlow flow,
        PortType type,
        unsigned index, 
        unsigned bufIndex
    ) : Port(info, lilvPort, flow, type, index),
        bufIndex(bufIndex) { }

CvBufPort::CvBufPort(
        PluginInfo *info, 
        const LilvPort *lilvPort, 
        PortFlow flow, 
        unsigned index, 
        unsigned bufIndex
    ) : BufPort(info, lilvPort, flow, PortType::CV, index, bufIndex) { }


AudioBufPort::AudioBufPort(
        PluginInfo *info, 
        const LilvPort *lilvPort, 
        PortFlow flow, 
        unsigned index, 
        unsigned bufIndex
    ) : BufPort(info, lilvPort, flow, PortType::Audio, index, bufIndex) { }


EventPort::EventPort(
        PluginInfo *info, 
        const LilvPort *lilvPort, 
        PortFlow flow,
        PortType type,
        unsigned index, 
        unsigned bufIndex
    ) : Port(info, lilvPort, flow, type, index),
        bufIndex(bufIndex) {

    // apiType = atomEventApi ? LV2_EVBUF_ATOM : LV2_EVBUF_EVENT;
 }


MidiPort::MidiPort(
        PluginInfo *info, 
        const LilvPort *lilvPort, 
        PortFlow portFlow, 
        unsigned index, 
        unsigned bufIndex
    ) : EventPort(info, lilvPort, portFlow, PortType::Midi, index, bufIndex) {}


ControlPort::ControlPort(
        PluginInfo *info, 
        const LilvPort *lilvPort, 
        PortFlow portFlow, 
        uint32_t index, 
        uint32_t dataIndex) 
    : Port(info, lilvPort, portFlow, PortType::Control, index), 
      dataIndex(dataIndex) {

    }


ParamPort::ParamPort(
        PluginInfo *info, 
        const LilvPort *lilvPort, 
        PortFlow portFlow, 
        uint32_t index, 
        uint32_t dataIndex, 
        uint32_t byteOffset
    ) : ControlPort(info, lilvPort, portFlow, index, dataIndex), 
        byteOffset(byteOffset) {
                                
    zzubParam = new zzub::parameter();

    zzubParam->flags = zzub::parameter_flag_state;

    LilvNode *default_val_node, *min_val_node, *max_val_node;
    lilv_port_get_range(info->lilvPlugin, lilvPort, &default_val_node, &min_val_node, &max_val_node);

    minVal = lilv::as_float(min_val_node);
    maxVal = lilv::as_float(max_val_node);
	defaultVal = lilv::as_float(default_val_node);

//    if(verbose) { printf("\nbuild port %s: ", name.c_str()); }

    if(defaultVal < minVal || defaultVal > maxVal) {
        defaultVal = (maxVal - minVal) / 2.0f;
    }

    LilvScalePoints *lilv_scale_points = lilv_port_get_scale_points(info->lilvPlugin, lilvPort);
    unsigned scale_points_size = lilv::scale_size(lilv_scale_points);

    if (LV2_IS_PORT_TOGGLED(properties) || LV2_IS_PORT_TRIGGER(properties)) {
//        if(verbose) { printf(" As switch param "); }
        zzubParam->set_switch();
        zzubParam->value_default = switch_value_off;
    } else if(LV2_IS_PORT_ENUMERATION(properties)) {
//        if(verbose) { printf(" As options list "); }
        (scale_points_size <= 128) ?  zzubParam->set_byte() : zzubParam->set_word();
        zzubParam->value_default = (int) defaultVal;
        zzubParam->value_max = scale_points_size;
    } else if (LV2_IS_PORT_INTEGER(properties)) {
//        if(verbose) { printf(" as integer val "); }
        (maxVal - minVal <= 128) ? zzubParam->set_byte() : zzubParam->set_word();
        zzubParam->value_default = (int) defaultVal;
        zzubParam->value_min = 0;
        zzubParam->value_max = std::min((int)(maxVal - minVal), 32768);
    } else {
//        if(verbose) { printf(" as word len slider "); }
        //TODO check if logarithm, check for other properties
        zzubParam->set_word();
        zzubParam->value_min = 0;
        zzubParam->value_max = 32768;
        zzubParam->value_default = lilv_to_zzub_value(defaultVal);
    }

    byteSize = zzubParam->get_bytesize();
    zzubParam->name = name.c_str();
    zzubParam->description = zzubParam->name;

    if(lilv_scale_points != NULL ) {
        LILV_FOREACH(scale_points, spIter,lilv_scale_points) {
            const LilvScalePoint *lilvScalePoint = lilv_scale_points_get(lilv_scale_points, spIter);
            scalePoints.push_back(new ScalePoint(lilvScalePoint));
            if(verbose) { printf("\nScale point %f, %s", scalePoints[scalePoints.size()-1]->value, scalePoints[scalePoints.size()-1]->label.c_str()); }
        }

        lilv_scale_points_free(lilv_scale_points);
    }
}


//-----------------------------------------------------------------------------------


uint32_t get_port_properties(const PluginWorld *world, const LilvPlugin *lilvPlugin, const LilvPort *lilvPort) {
    uint32_t properties = 0;
    
    if (lilv_port_has_property(lilvPlugin, lilvPort, world->pprop_optional))
        properties |= LV2_PORT_OPTIONAL;
    if (lilv_port_has_property(lilvPlugin, lilvPort, world->pprop_enumeration))
        properties |= LV2_PORT_ENUMERATION;
    if (lilv_port_has_property(lilvPlugin, lilvPort, world->pprop_integer))
        properties |= LV2_PORT_INTEGER;
    if (lilv_port_has_property(lilvPlugin, lilvPort, world->pprop_sampleRate))
        properties |= LV2_PORT_SAMPLE_RATE;
    if (lilv_port_has_property(lilvPlugin, lilvPort, world->pprop_toggled))
        properties |= LV2_PORT_TOGGLED;

    if (lilv_port_has_property(lilvPlugin, lilvPort, world->pprop_artifacts) || lilv_port_has_property(lilvPlugin, lilvPort, world->oldPropArtifacts))
        properties |= LV2_PORT_CAUSES_ARTIFACTS;
    if (lilv_port_has_property(lilvPlugin, lilvPort, world->pprop_continuousCV) || lilv_port_has_property(lilvPlugin, lilvPort, world->oldPropContinuousCV))
        properties |= LV2_PORT_CONTINUOUS_CV;
    if (lilv_port_has_property(lilvPlugin, lilvPort, world->pprop_discreteCV) || lilv_port_has_property(lilvPlugin, lilvPort, world->oldPropDiscreteCV))
        properties |= LV2_PORT_DISCRETE_CV;
    if (lilv_port_has_property(lilvPlugin, lilvPort, world->pprop_expensive) || lilv_port_has_property(lilvPlugin, lilvPort, world->oldPropExpensive))
        properties |= LV2_PORT_EXPENSIVE;
    if (lilv_port_has_property(lilvPlugin, lilvPort, world->pprop_strictBounds) || lilv_port_has_property(lilvPlugin, lilvPort, world->oldPropStrictBounds))
        properties |= LV2_PORT_STRICT_BOUNDS;
    if (lilv_port_has_property(lilvPlugin, lilvPort, world->pprop_logarithmic) || lilv_port_has_property(lilvPlugin, lilvPort, world->oldPropLogarithmic))
        properties |= LV2_PORT_LOGARITHMIC;
    if (lilv_port_has_property(lilvPlugin, lilvPort, world->pprop_notAutomatic) || lilv_port_has_property(lilvPlugin, lilvPort, world->oldPropNotAutomatic))
        properties |= LV2_PORT_NOT_AUTOMATIC;
    if (lilv_port_has_property(lilvPlugin, lilvPort, world->pprop_notOnGUI) || lilv_port_has_property(lilvPlugin, lilvPort, world->oldPropNotOnGUI))
        properties |= LV2_PORT_NOT_ON_GUI;
    if (lilv_port_has_property(lilvPlugin, lilvPort, world->pprop_trigger) || lilv_port_has_property(lilvPlugin, lilvPort, world->oldPropTrigger))
        properties |= LV2_PORT_TRIGGER;
    if (lilv_port_has_property(lilvPlugin, lilvPort, world->pprop_nonAutomable))
        properties |= LV2_PORT_NON_AUTOMABLE;

    return properties;
}


//-----------------------------------------------------------------------------------


uint32_t get_port_designation(const PluginWorld *world, const LilvPlugin *lilvPlugin, const LilvPort *lilvPort) {
    uint32_t designation = 0;

    if (lilv_port_has_property(lilvPlugin, lilvPort, world->reportsLatency))
        designation |= LV2_PORT_DESIGNATION_LATENCY;

    LilvNode* const designationNode = lilv_port_get(lilvPlugin, lilvPort, world->designation);

    if (designationNode) {
        std::string designationStr = lilv::as_string(designationNode);

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
                        lilv::as_string(lilv_plugin_get_name(lilvPlugin), true).c_str(),
                        lilv::as_string(lilv_port_get_name(lilvPlugin, lilvPort), true).c_str(),
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

//     if (lilv::nodes_size(typeNodes) > 0) {
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



