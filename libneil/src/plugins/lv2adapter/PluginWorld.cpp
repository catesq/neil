#include "PluginWorld.h"
#include "lv2_utils.h"

//-----------------------------------------------------------------------------------

extern "C" {

    LV2_URID map_uri(LV2_URID_Map_Handle handle, const char *uri) {
        PluginWorld* world = (PluginWorld*)handle;
        const LV2_URID id = symap_map(world->symap, uri);
        return id;
    }

    const char* unmap_uri(LV2_URID_Unmap_Handle handle, LV2_URID urid) {
        PluginWorld* world = (PluginWorld*)handle;
        const char* uri = symap_unmap(world->symap, urid);
        return uri;
    }


    /** Map function for URI map extension. */
    uint32_t uri_to_id(LV2_URI_Map_Callback_Data callback_data, const char *map, const char *uri) {
        PluginWorld* world = (PluginWorld*)callback_data;
        const LV2_URID id = symap_map(world->symap, uri);
        return id;
    }

    char* lv2_make_path(LV2_State_Make_Path_Handle handle, const char *path) {
        PluginWorld *world = (PluginWorld*)handle;

        // Create in save directory if saving, otherwise use temp directory
        const char* dir = world->hostParams.tempDir;

        int fname_len = strlen(dir) + strlen(path) + 2;

        if (fname_len > 1024) return NULL;

        char* fullpath = (char*) malloc(fname_len);
        strcpy(fullpath, dir);
        strcat(fullpath, FILESEP);
        strcat(fullpath, path);

    //    fprintf(stderr, "MAKE PATH `%s' => `%s'\n", path, fullpath);

        return fullpath;
    }

}

//-----------------------------------------------------------------------------------



PluginWorld::~PluginWorld() {
    if(hostParams.tempDir != nullptr) {
        free(hostParams.tempDir);
        hostParams.tempDir = nullptr;
    }
}


PluginWorld::PluginWorld()
: lilvWorld(lilv_world_new()),
  symap             (symap_new()),
  port              (new_uri(LV2_CORE__port)),
  symbol            (new_uri(LV2_CORE__symbol)),
  designation       (new_uri(LV2_CORE__designation)),
  freeWheeling      (new_uri(LV2_CORE__freeWheeling)),
  reportsLatency    (new_uri(LV2_CORE__reportsLatency)),

//   class_allpass     (new_uri(LV2_CORE__AllpassPlugin)),
//   class_amplifier   (new_uri(LV2_CORE__AmplifierPlugin)),
//   class_analyzer    (new_uri(LV2_CORE__AnalyserPlugin)),
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

  port_input         (new_uri(LV2_CORE__InputPort)),
  port_output        (new_uri(LV2_CORE__OutputPort)),
  port_control       (new_uri(LV2_CORE__ControlPort)),
  port_audio         (new_uri(LV2_CORE__AudioPort)),
  port_cv            (new_uri(LV2_CORE__CVPort)),
  port_atom          (new_uri(LV2_ATOM__AtomPort)),
  port_event         (new_uri(LV2_EVENT__EventPort)),
  port_midi          (new_uri(LV2_MIDI_LL__MidiPort)),

  pprop_optional     (new_uri(LV2_CORE__connectionOptional)),
  pprop_enumeration  (new_uri(LV2_CORE__enumeration)),
  pprop_integer      (new_uri(LV2_CORE__integer)),
  pprop_sampleRate   (new_uri(LV2_CORE__sampleRate)),
  pprop_toggled      (new_uri(LV2_CORE__toggled)),
  pprop_artifacts    (new_uri(LV2_PORT_PROPS__causesArtifacts)),
  pprop_continuousCV (new_uri(LV2_PORT_PROPS__continuousCV)),
  pprop_discreteCV   (new_uri(LV2_PORT_PROPS__discreteCV)),
  pprop_expensive    (new_uri(LV2_PORT_PROPS__expensive)),
  pprop_strictBounds (new_uri(LV2_PORT_PROPS__hasStrictBounds)),
  pprop_logarithmic  (new_uri(LV2_PORT_PROPS__logarithmic)),
  pprop_notAutomatic (new_uri(LV2_PORT_PROPS__notAutomatic)),
  pprop_notOnGUI     (new_uri(LV2_PORT_PROPS__notOnGUI)),
  pprop_trigger      (new_uri(LV2_PORT_PROPS__trigger)),
  pprop_nonAutomable (new_uri(LV2_KXSTUDIO_PROPERTIES__NonAutomable)),

  oldPropArtifacts    (new_uri("http://lv2plug.in/ns/dev/extportinfo#causesArtifacts")),
  oldPropContinuousCV (new_uri("http://lv2plug.in/ns/dev/extportinfo#continuousCV")),
  oldPropDiscreteCV   (new_uri("http://lv2plug.in/ns/dev/extportinfo#discreteCV")),
  oldPropExpensive    (new_uri("http://lv2plug.in/ns/dev/extportinfo#expensive")),
  oldPropStrictBounds (new_uri("http://lv2plug.in/ns/dev/extportinfo#hasStrictBounds")),
  oldPropLogarithmic  (new_uri("http://lv2plug.in/ns/dev/extportinfo#logarithmic")),
  oldPropNotAutomatic (new_uri("http://lv2plug.in/ns/dev/extportinfo#notAutomatic")),
  oldPropNotOnGUI     (new_uri("http://lv2plug.in/ns/dev/extportinfo#notOnGUI")),
  oldPropTrigger      (new_uri("http://lv2plug.in/ns/dev/extportinfo#trigger")),

  unit_name          (new_uri(LV2_UNITS__name)),
  unit_render        (new_uri(LV2_UNITS__render)),
  unit_symbol        (new_uri(LV2_UNITS__symbol)),
  unit_unit          (new_uri(LV2_UNITS__unit)),

  ui_gtk2            (new_uri(LV2_UI__GtkUI)),
  ui_gtk3            (new_uri(LV2_UI__Gtk3UI)),
  ui_qt4             (new_uri(LV2_UI__Qt4UI)),
  ui_qt5             (new_uri(LV2_UI__Qt5UI)),
  ui_cocoa           (new_uri(LV2_UI__CocoaUI)),
  ui_windows         (new_uri(LV2_UI__WindowsUI)),
  ui_x11             (new_uri(LV2_UI__X11UI)),
  ui_external        (new_uri(LV2_EXTERNAL_UI__Widget)),

  atom_bufferType    (new_uri(LV2_ATOM__bufferType)),
  atom_chunk         (new_uri(LV2_ATOM__Chunk)),
  atom_sequence      (new_uri(LV2_ATOM__Sequence)),
  atom_supports      (new_uri(LV2_ATOM__supports)),

  preset_preset      (new_uri(LV2_PRESETS__Preset)),

  state_state        (new_uri(LV2_STATE__state)),

  value_default      (new_uri(LV2_CORE__default)),
  value_minimum      (new_uri(LV2_CORE__minimum)),
  value_maximum      (new_uri(LV2_CORE__maximum)),

  midi_event         (new_uri(LV2_MIDI__MidiEvent)),
  patch_message      (new_uri(LV2_PATCH__Message)),
  time_position      (new_uri(LV2_TIME__Position)),

  mm_defaultControl  (new_uri(NS_llmm "defaultMidiController")),
  mm_controlType     (new_uri(NS_llmm "controllerType")),
  mm_controlNumber   (new_uri(NS_llmm "controllerNumber")),

//   dct_replaces        (new_uri(NS_dct "replaces")),
//   doap_license        (new_uri(NS_doap "license")),
  rdf_type            (new_uri(NS_rdf "type")),
  suil_is_init(false)
//   rdfs_label          (new_uri(NS_rdfs "label"))

  {

	lilv_world_load_all(lilvWorld);
    


    urids.atom_Float = symap_map(symap, LV2_ATOM__Float);
    urids.atom_Int = symap_map(symap, LV2_ATOM__Int);
    urids.atom_Double = symap_map(symap, LV2_ATOM__Double);
    urids.atom_Long = symap_map(symap, LV2_ATOM__Long);
    urids.atom_Bool = symap_map(symap, LV2_ATOM__Bool);
    urids.atom_Chunk = symap_map(symap, LV2_ATOM__Chunk);
    urids.atom_Sequence = symap_map(symap, LV2_ATOM__Sequence);
    urids.atom_eventTransfer = symap_map(symap, LV2_ATOM__eventTransfer);
    urids.maxBlockLength = symap_map(symap, LV2_BUF_SIZE__maxBlockLength);
    urids.minBlockLength = symap_map(symap, LV2_BUF_SIZE__minBlockLength);
    urids.bufSeqSize = symap_map(symap, LV2_BUF_SIZE__sequenceSize);
    urids.nominalBlockLength = symap_map(symap, LV2_BUF_SIZE__nominalBlockLength);
    urids.log_Trace = symap_map(symap, LV2_LOG__Trace);
    urids.midi_MidiEvent = symap_map(symap, LV2_MIDI__MidiEvent);
    urids.param_sampleRate = symap_map(symap, LV2_PARAMETERS__sampleRate);
    urids.patch_Set = symap_map(symap, LV2_PATCH__Set);
    urids.patch_property = symap_map(symap, LV2_PATCH__property);
    urids.patch_value = symap_map(symap, LV2_PATCH__value);
    urids.time_Position = symap_map(symap, LV2_TIME__Position);
    urids.time_bar = symap_map(symap, LV2_TIME__bar);
    urids.time_barBeat = symap_map(symap, LV2_TIME__barBeat);
    urids.time_beatUnit = symap_map(symap, LV2_TIME__beatUnit);
    urids.time_beatsPerBar = symap_map(symap, LV2_TIME__beatsPerBar);
    urids.time_beatsPerMinute = symap_map(symap, LV2_TIME__beatsPerMinute);
    urids.time_frame = symap_map(symap, LV2_TIME__frame);
    urids.time_speed = symap_map(symap, LV2_TIME__speed);
    urids.ui_updateRate = symap_map(symap, LV2_UI__updateRate);

    


    map.handle  = &(*this);
    map.map     = &map_uri;
    unmap.handle  = &(*this);
    unmap.unmap   = unmap_uri;
    make_path.handle = this;
    make_path.path = &lv2_make_path;
    uri_map.callback_data = this;
    uri_map.uri_to_id = &uri_to_id;

    lv2_atom_forge_init(&forge, &map);


//     static LV2_Options_Option lv2Options[] = {
//        { LV2_OPTIONS_INSTANCE, 0, urids.param_sampleRate,   sizeof(float), urids.atom_Float, &hostParams.sample_rate },
//        { LV2_OPTIONS_INSTANCE, 0, urids.minBlockLength,     sizeof(int32_t), urids.atom_Int, &hostParams.blockLength },
//        { LV2_OPTIONS_INSTANCE, 0, urids.maxBlockLength,     sizeof(int32_t), urids.atom_Int, &hostParams.blockLength },
//        { LV2_OPTIONS_INSTANCE, 0, urids.nominalBlockLength, sizeof(int32_t), urids.atom_Int, &hostParams.blockLength },
//        { LV2_OPTIONS_INSTANCE, 0, urids.midiSeqSize,        sizeof(int32_t), urids.atom_Int, &hostParams.evbufMinSize },

//        { LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, NULL }
//     };

//     uri_map.callback_data = &(*this);




//     lv2_atom_forge_init(&forge, &map);

//     options_feature.data = &lv2Options;

#ifdef _WIN32
    char tempdir[] = "lilvXXXXXX";
    char *dtmp_res = _mktemp(tempdir);
#else
    char tempdir[] = "/tmp/lilvZ-XXXXXX";
    char *dtmp_res = mkdtemp(tempdir);
#endif

    if(dtmp_res == nullptr) {
        tempdir[0] = '\0';
        dtmp_res = &tempdir[0];
    }

    unsigned dtmp_len = strlen(dtmp_res) + 1;
    hostParams.tempDir = (char*) malloc(dtmp_len);
    strncpy(hostParams.tempDir, dtmp_res, dtmp_len);

//     LV2_State_Make_Path make_path = { &(*this), lv2_make_path };
//     make_path_feature.data = &make_path;

//    LV2_Worker_Schedule schedule = { &(*this), jalv_worker_schedule };
//    schedule_feature.data = &schedule;


}

void PluginWorld::init_suil() {
    if(!suil_is_init) {
        suil_is_init = true;
        suil_init(0, NULL, SUIL_ARG_NONE);
    }
}



void PluginWorld::setSampleRate(float sample_rate) {
    hostParams.sample_rate = sample_rate;
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
