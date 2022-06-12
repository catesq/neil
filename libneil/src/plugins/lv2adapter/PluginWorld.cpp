#include "PluginWorld.h"
#include "lv2_utils.h"
#include "lilv/lilv.h"
#include <X11/Xlib.h>


//-----------------------------------------------------------------------------------

extern "C" {

    LV2_URID map_uri(LV2_URID_Map_Handle handle, const char *uri) {
        SharedAdapterCache* cache = (SharedAdapterCache*)handle;
        const LV2_URID id = symap_map(cache->symap, uri);
//        printf("mapped %u from %s\n", id, uri);
        return id;
    }

    const char* unmap_uri(LV2_URID_Unmap_Handle handle, LV2_URID urid) {
        SharedAdapterCache* cache = (SharedAdapterCache*)handle;
        const char* uri = symap_unmap(cache->symap, urid);
//        printf("unmapped %u to %s\n", urid, uri);
        return uri;
    }

    char* lv2_make_path(LV2_State_Make_Path_Handle handle, const char *path) {
        SharedAdapterCache *cache = (SharedAdapterCache*)handle;
        std::string fname = cache->hostParams.tempDir + FILE_SEPARATOR + std::string(path);
        return strdup(fname.c_str());
    }

}

//-----------------------------------------------------------------------------------

Nodes::Nodes(LilvWorld* world)
    : port                (lilv_new_uri(world, LV2_CORE__port)),
      symbol              (lilv_new_uri(world, LV2_CORE__symbol)),
      designation         (lilv_new_uri(world, LV2_CORE__designation)),
      freeWheeling        (lilv_new_uri(world, LV2_CORE__freeWheeling)),
      reportsLatency      (lilv_new_uri(world, LV2_CORE__reportsLatency)),
      port_input          (lilv_new_uri(world, LV2_CORE__InputPort)),
      port_output         (lilv_new_uri(world, LV2_CORE__OutputPort)),
      port_control        (lilv_new_uri(world, LV2_CORE__ControlPort)),
      port_audio          (lilv_new_uri(world, LV2_CORE__AudioPort)),
      port_cv             (lilv_new_uri(world, LV2_CORE__CVPort)),
      port_atom           (lilv_new_uri(world, LV2_ATOM__AtomPort)),
      port_event          (lilv_new_uri(world, LV2_EVENT__EventPort)),
      port_midi           (lilv_new_uri(world, LV2_MIDI_LL__MidiPort)),
      pprop_optional      (lilv_new_uri(world, LV2_CORE__connectionOptional)),
      pprop_enumeration   (lilv_new_uri(world, LV2_CORE__enumeration)),
      pprop_integer       (lilv_new_uri(world, LV2_CORE__integer)),
      pprop_sampleRate    (lilv_new_uri(world, LV2_CORE__sampleRate)),
      pprop_toggled       (lilv_new_uri(world, LV2_CORE__toggled)),
      pprop_artifacts     (lilv_new_uri(world, LV2_PORT_PROPS__causesArtifacts)),
      pprop_continuousCV  (lilv_new_uri(world, LV2_PORT_PROPS__continuousCV)),
      pprop_discreteCV    (lilv_new_uri(world, LV2_PORT_PROPS__discreteCV)),
      pprop_expensive     (lilv_new_uri(world, LV2_PORT_PROPS__expensive)),
      pprop_strictBounds  (lilv_new_uri(world, LV2_PORT_PROPS__hasStrictBounds)),
      pprop_logarithmic   (lilv_new_uri(world, LV2_PORT_PROPS__logarithmic)),
      pprop_notAutomatic  (lilv_new_uri(world, LV2_PORT_PROPS__notAutomatic)),
      pprop_notOnGUI      (lilv_new_uri(world, LV2_PORT_PROPS__notOnGUI)),
      pprop_trigger       (lilv_new_uri(world, LV2_PORT_PROPS__trigger)),
      pprop_nonAutomable  (lilv_new_uri(world, LV2_KXSTUDIO_PROPERTIES__NonAutomable)),
      oldPropArtifacts    (lilv_new_uri(world, "http://lv2plug.in/ns/dev/extportinfo#causesArtifacts")),
      oldPropContinuousCV (lilv_new_uri(world, "http://lv2plug.in/ns/dev/extportinfo#continuousCV")),
      oldPropDiscreteCV   (lilv_new_uri(world, "http://lv2plug.in/ns/dev/extportinfo#discreteCV")),
      oldPropExpensive    (lilv_new_uri(world, "http://lv2plug.in/ns/dev/extportinfo#expensive")),
      oldPropStrictBounds (lilv_new_uri(world, "http://lv2plug.in/ns/dev/extportinfo#hasStrictBounds")),
      oldPropLogarithmic  (lilv_new_uri(world, "http://lv2plug.in/ns/dev/extportinfo#logarithmic")),
      oldPropNotAutomatic (lilv_new_uri(world, "http://lv2plug.in/ns/dev/extportinfo#notAutomatic")),
      oldPropNotOnGUI     (lilv_new_uri(world, "http://lv2plug.in/ns/dev/extportinfo#notOnGUI")),
      oldPropTrigger      (lilv_new_uri(world, "http://lv2plug.in/ns/dev/extportinfo#trigger")),

      ui_gtk2             (lilv_new_uri(world, LV2_UI__GtkUI)),
      ui_gtk3             (lilv_new_uri(world, LV2_UI__Gtk3UI)),
      ui_qt4              (lilv_new_uri(world, LV2_UI__Qt4UI)),
      ui_qt5              (lilv_new_uri(world, LV2_UI__Qt5UI)),
      ui_cocoa            (lilv_new_uri(world, LV2_UI__CocoaUI)),
      ui_windows          (lilv_new_uri(world, LV2_UI__WindowsUI)),
      ui_x11              (lilv_new_uri(world, LV2_UI__X11UI)),
      ui_external         (lilv_new_uri(world, LV2_EXTERNAL_UI__Widget)),
      atom_bufferType     (lilv_new_uri(world, LV2_ATOM__bufferType)),
      atom_chunk          (lilv_new_uri(world, LV2_ATOM__Chunk)),
      atom_sequence       (lilv_new_uri(world, LV2_ATOM__Sequence)),
      atom_supports       (lilv_new_uri(world, LV2_ATOM__supports)),
      state_state         (lilv_new_uri(world, LV2_STATE__state)),
      value_default       (lilv_new_uri(world, LV2_CORE__default)),
      value_minimum       (lilv_new_uri(world, LV2_CORE__minimum)),
      value_maximum       (lilv_new_uri(world, LV2_CORE__maximum)),
      midi_event          (lilv_new_uri(world, LV2_MIDI__MidiEvent)),
      time_position       (lilv_new_uri(world, LV2_TIME__Position)),
      mm_defaultControl   (lilv_new_uri(world, NS_llmm "defaultMidiController")),
      mm_controlType      (lilv_new_uri(world, NS_llmm "controllerType")),
      mm_controlNumber    (lilv_new_uri(world, NS_llmm "controllerNumber")),
      worker_iface        (lilv_new_uri(world, LV2_WORKER__interface)),
      worker_schedule     (lilv_new_uri(world, LV2_WORKER__schedule)),
      extensionData       (lilv_new_uri(world, LV2_CORE__extensionData)),
      showInterface       (lilv_new_uri(world, LV2_UI__showInterface)),
      rdf_type            (lilv_new_uri(world, NS_rdf "type")) {
}

Nodes::~Nodes() {
    LilvNode** node = &this->port;
    do {
        lilv_node_free(*node);

    } while(*++node != this->rdf_type);
}


SharedAdapterCache::~SharedAdapterCache() {

}

//   class_bandpass    (new_uri(LV2_CORE__BandpassPlugin)),
//   class_chorus      (new_uri(LV2_CORE__ChorusPlugin)),
//   class_comb        (new_uri(LV2_CORE__CombPlugin)),
//   class_compressor  (new_uri(LV2_CORE__CompressorPlugin)),
//   class_constant    (new_uri(LV2_CORE__ConstantPlugin)),
//   class_converter   (new_uri(LV2_CORE__ConverterPlugin)),
//   class_delay       (new_uri(LV2_CORE__DelayPlugin)),
//   class_distortion  (new_uri(LV2_CORE__DistortionPlugin)),
//   class_dynamics    (new_uri(LV2_CORE__DynamicsPlugin)),
//   class_eq          (new_uri(LV2_CORE__EQPlugin)),
//   class_envelope    (new_uri(LV2_CORE__EnvelopePlugin)),
//   class_expander    (new_uri(LV2_CORE__ExpanderPlugin)),
//   class_filter      (new_uri(LV2_CORE__FilterPlugin)),
//   class_flanger     (new_uri(LV2_CORE__FlangerPlugin)),
//   class_function    (new_uri(LV2_CORE__FunctionPlugin)),
//   class_gate        (new_uri(LV2_CORE__GatePlugin)),
//   class_generator   (new_uri(LV2_CORE__GeneratorPlugin)),
//   class_highpass    (new_uri(LV2_CORE__HighpassPlugin)),
//   class_instrument  (new_uri(LV2_CORE__InstrumentPlugin)),
//   class_limiter     (new_uri(LV2_CORE__LimiterPlugin)),
//   class_lowpass     (new_uri(LV2_CORE__LowpassPlugin)),
//   class_mixer       (new_uri(LV2_CORE__MixerPlugin)),
//   class_modulator   (new_uri(LV2_CORE__ModulatorPlugin)),
//   class_multiEQ     (new_uri(LV2_CORE__MultiEQPlugin)),
//   class_oscillator  (new_uri(LV2_CORE__OscillatorPlugin)),
//   class_paraEQ      (new_uri(LV2_CORE__ParaEQPlugin)),
//   class_phaser      (new_uri(LV2_CORE__PhaserPlugin)),
//   class_pitch       (new_uri(LV2_CORE__PitchPlugin)),
//   class_reverb      (new_uri(LV2_CORE__ReverbPlugin)),
//   class_simulator   (new_uri(LV2_CORE__SimulatorPlugin)),
//   class_spatial     (new_uri(LV2_CORE__SpatialPlugin)),
//   class_spectral    (new_uri(LV2_CORE__SpectralPlugin)),
//   class_utility     (new_uri(LV2_CORE__UtilityPlugin)),
//   class_waveshaper  (new_uri(LV2_CORE__WaveshaperPlugin)),
//   unit_name          (new_uri(LV2_UNITS__name)),
//   unit_render        (new_uri(LV2_UNITS__render)),
//   unit_symbol        (new_uri(LV2_UNITS__symbol)),
//   unit_unit          (new_uri(LV2_UNITS__unit)),
//   preset_preset      (new_uri(LV2_PRESETS__Preset)),
//   patch_message      (new_uri(LV2_PATCH__Message)),

SharedAdapterCache::SharedAdapterCache()
    : lilvWorld(lilv_world_new()),
      nodes(lilvWorld),
      symap(symap_new()),
      suil_is_init(false)  {

    lilv_world_load_all(lilvWorld);
    
    urids.atom_Float           = symap_map(symap, LV2_ATOM__Float);
    urids.atom_Int             = symap_map(symap, LV2_ATOM__Int);
    urids.atom_Double          = symap_map(symap, LV2_ATOM__Double);
    urids.atom_Long            = symap_map(symap, LV2_ATOM__Long);
    urids.atom_Bool            = symap_map(symap, LV2_ATOM__Bool);
    urids.atom_Chunk           = symap_map(symap, LV2_ATOM__Chunk);
    urids.atom_Sequence        = symap_map(symap, LV2_ATOM__Sequence);
    urids.atom_eventTransfer   = symap_map(symap, LV2_ATOM__eventTransfer);
    urids.maxBlockLength       = symap_map(symap, LV2_BUF_SIZE__maxBlockLength);
    urids.minBlockLength       = symap_map(symap, LV2_BUF_SIZE__minBlockLength);
    urids.bufSeqSize           = symap_map(symap, LV2_BUF_SIZE__sequenceSize);
    urids.nominalBlockLength   = symap_map(symap, LV2_BUF_SIZE__nominalBlockLength);
    // urids.log_Trace = symap_map(symap, LV2_LOG__Trace);
    urids.midi_MidiEvent       = symap_map(symap, LV2_MIDI__MidiEvent);
    urids.param_sampleRate     = symap_map(symap, LV2_PARAMETERS__sampleRate);
    // urids.patch_Set = symap_map(symap, LV2_PATCH__Set);
    // urids.patch_property = symap_map(symap, LV2_PATCH__property);
    // urids.patch_value = symap_map(symap, LV2_PATCH__value);
    urids.time_Position        = symap_map(symap, LV2_TIME__Position);
    urids.time_bar             = symap_map(symap, LV2_TIME__bar);
    urids.time_barBeat         = symap_map(symap, LV2_TIME__barBeat);
    urids.time_beatUnit        = symap_map(symap, LV2_TIME__beatUnit);
    urids.time_beatsPerBar     = symap_map(symap, LV2_TIME__beatsPerBar);
    urids.time_beatsPerMinute  = symap_map(symap, LV2_TIME__beatsPerMinute);
    urids.time_frame           = symap_map(symap, LV2_TIME__frame);
    urids.time_speed           = symap_map(symap, LV2_TIME__speed);
    urids.ui_updateRate        = symap_map(symap, LV2_UI__updateRate);
    urids.ui_scaleFactor       = symap_map(symap, LV2_UI__scaleFactor);
    urids.ui_transientWindowId = symap_map(symap, LV2_KXSTUDIO_PROPERTIES__TransientWindowId);

    map.handle  = &(*this);
    map.map     = &map_uri;
    unmap.handle  = &(*this);
    unmap.unmap   = unmap_uri;
    make_path.handle = this;
    make_path.path = &lv2_make_path;

//    uri_map.callback_data = this;
//    uri_map.uri_to_id = &uri_to_id;

    lv2_atom_forge_init(&forge, &map);

//     uri_map.callback_data = &(*this);

//     lv2_atom_forge_init(&forge, &map);


#ifdef _WIN32
    char tempdir[] = "lilv-neil-XXXXXX";
    char *dtmp_res = _mktemp(tempdir);
#else
    char tempdir[] = "/tmp/lilv-neil-XXXXXX";
    char *dtmp_res = mkdtemp(tempdir);
#endif

    if(dtmp_res == nullptr) {
        tempdir[0] = '\0';
        dtmp_res = &tempdir[0];
    }

    hostParams.tempDir = std::string(dtmp_res);

}

void SharedAdapterCache::init_suil() {
    suil_mtx.lock();

    if(!suil_is_init) {
        suil_is_init = true;
        suil_init(0, NULL, SUIL_ARG_NONE);
    }
    suil_mtx.unlock();
}




// const LV2_Feature** PluginWorld::getLv2Features() {

//      LV2_Options_Option options[NUM_OPTIONS] = {
//        { LV2_OPTIONS_INSTANCE, 0, urids.param_sampleRate,   sizeof(float), urids.atom_Float, &hostParams.sample_rate },
//        { LV2_OPTIONS_INSTANCE, 0, urids.minBlockLength,     sizeof(int32_t), urids.atom_Int, &hostParams.blockLength },
//        { LV2_OPTIONS_INSTANCE, 0, urids.maxBlockLength,     sizeof(int32_t), urids.atom_Int, &hostParams.blockLength },
//        { LV2_OPTIONS_INSTANCE, 0, urids.nominalBlockLength, sizeof(int32_t), urids.atom_Int, &hostParams.blockLength },
//        { LV2_OPTIONS_INSTANCE, 0, urids.bufSeqSize,        sizeof(int32_t), urids.atom_Int, &hostParams.evbufMinSize },

//        { LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, NULL }
//     };

//     memcpy(features.options, options, sizeof(features.options));

//     features.uri_map.callback_data = this;

//     features.map.handle  = this;
//     features.map.map     = &map_uri;
//     features.map_feature.data = &map;

//     features.unmap.handle  = this;
//     features.unmap.unmap   = unmap_uri;
//     features.unmap_feature.data = &unmap;

//     lv2_atom_forge_init(&forge, &map);

//     features.options_feature.data = features.options;

// #ifdef _WIN32
//     char tempdir[] = "lilvXXXXXX";
//     char *dtmp_res = _mktemp(tempdir);
// #else
//     char tempdir[] = "/tmp/lilvZ-XXXXXX";
//     char *dtmp_res = mkdtemp(tempdir);
// #endif

//     if(dtmp_res == nullptr) {
//         tempdir[0] = '\0';
//         dtmp_res = &tempdir[0];
//     }

//     unsigned dtmp_len = strlen(dtmp_res) + 1;
//     hostParams.tempDir = (char*) malloc(dtmp_len);
//     strncpy(hostParams.tempDir, dtmp_res, dtmp_len);

//     LV2_State_Make_Path make_path = { this->world, lv2_make_path };
//     make_path_feature.data = &make_path;

//     const LV2_Feature* const features[] = {
// 		&>features.map_feature,
// 		&features.unmap_feature,
// 		&features.sched_feature,
// 		&jalv->features.options_feature,
// 		&static_features[0],
// 		&static_features[1],
// 		&static_features[2],
// 		&static_features[3],
// 		NULL
// 	};

// }
