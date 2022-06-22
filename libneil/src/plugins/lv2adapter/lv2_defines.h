#pragma once

static int verbose = 0; 

#include <string>

#include "zzub/zzub.h"
#include "lilv/lilv.h"

#include "lv2.h"

#include "lv2/options/options.h"
#include "lv2/data-access/data-access.h"
#include "lv2/uri-map/uri-map.h"
#include "lv2/state/state.h"
#include "lv2/buf-size/buf-size.h"
#include "lv2/midi/midi.h"
#include "lv2/atom/forge.h"
#include "lv2/parameters/parameters.h"
#include "lv2/time/time.h"
#include "lv2/port-groups/port-groups.h"
#include "lv2/event/event.h"
#include "lv2/worker/worker.h"
#include "lv2/instance-access/instance-access.h"

#include "lv2/port-props/port-props.h"

#include "ext/lv2_programs.h"

#define ZZUB_BUFLEN zzub_buffer_size
#define EVENT_BUF_CYCLES 8
#define EVENT_BUF_SIZE ZZUB_BUFLEN * EVENT_BUF_CYCLES 
#define TRACKVAL_VOLUME_UNDEFINED 0x0FF
#define TRACKVAL_NO_MIDI_CMD 0x00
#define TRACKVAL_NO_MIDI_DATA 0xFFFF

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define F_THRESHOLD 0.00001
#define SIMILAR(a, b) (a > b - F_THRESHOLD && a < b + F_THRESHOLD)
#define GTK3_URI "http://lv2plug.in/ns/extensions/ui#Gtk3UI"

#define MAX_PATHLEN 1024

// -----------------------------------------------------------------------

typedef struct {
    uint32_t index;
    uint32_t protocol;
    uint32_t size;
    uint8_t  body[];
} ControlChange;


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

struct Lv2HostParams {
    int32_t     blockLength = ZZUB_BUFLEN;
    int32_t     minBlockLength = 16;
    int32_t     bufSize = EVENT_BUF_SIZE;
    std::string tempDir = "";
};

typedef struct _LV2Features {
    LV2_Feature                map_feature;
    LV2_Feature                unmap_feature;
    LV2_Feature                bounded_buf_feature;
    LV2_Feature                make_path_feature ;
    LV2_Feature                options_feature;
    LV2_Feature                data_access_feature;
    LV2_Feature                program_host_feature;
    LV2_Feature                worker_feature;

    LV2_Feature                ui_instance_feature;
    LV2_Feature                ui_parent_feature;
    LV2_Feature                ui_data_access_feature;
    LV2_Feature                ui_idle_feature;

    void*                      options;
    LV2_Extension_Data_Feature ext_data{nullptr};
    LV2_Worker_Schedule        worker_schedule;
    LV2_Feature                default_state_feature;
    LV2_Programs_Host          program_host;
} LV2Features;


// -----------------------------------------------------------------------



//-----------------------------------------------------------------------

#define NS_EXT "http://lv2plug.in/ns/ext/"

#define LV2_KXSTUDIO_PROPERTIES_URI    "http://kxstudio.sf.net/ns/lv2ext/props"

#define LV2_KXSTUDIO_PROPERTIES_PREFIX LV2_KXSTUDIO_PROPERTIES_URI "#"

#define LV2_PROPERTY u_int32_t

// Base Types
typedef const char* LV2_URI;
typedef uint32_t LV2_Property;

// Port Midi Map Types
#define LV2_PORT_MIDI_MAP_CC             1
#define LV2_PORT_MIDI_MAP_NRPN           2

#define LV2_IS_PORT_MIDI_MAP_CC(x)       ((x) == LV2_PORT_MIDI_MAP_CC)
#define LV2_IS_PORT_MIDI_MAP_NRPN(x)     ((x) == LV2_PORT_MIDI_MAP_NRPN)

// Port Point Hints
#define LV2_PORT_POINT_DEFAULT           0x1
#define LV2_PORT_POINT_MINIMUM           0x2
#define LV2_PORT_POINT_MAXIMUM           0x4

#define LV2_HAVE_DEFAULT_PORT_POINT(x)   ((x) & LV2_PORT_POINT_DEFAULT)
#define LV2_HAVE_MINIMUM_PORT_POINT(x)   ((x) & LV2_PORT_POINT_MINIMUM)
#define LV2_HAVE_MAXIMUM_PORT_POINT(x)   ((x) & LV2_PORT_POINT_MAXIMUM)

// Port Unit Hints
#define LV2_PORT_UNIT_NAME               0x1
#define LV2_PORT_UNIT_RENDER             0x2
#define LV2_PORT_UNIT_SYMBOL             0x4
#define LV2_PORT_UNIT_UNIT               0x8

#define LV2_HAVE_PORT_UNIT_NAME(x)       ((x) & LV2_PORT_UNIT_NAME)
#define LV2_HAVE_PORT_UNIT_RENDER(x)     ((x) & LV2_PORT_UNIT_RENDER)
#define LV2_HAVE_PORT_UNIT_SYMBOL(x)     ((x) & LV2_PORT_UNIT_SYMBOL)
#define LV2_HAVE_PORT_UNIT_UNIT(x)       ((x) & LV2_PORT_UNIT_UNIT)

// Port Unit Unit
#define LV2_PORT_UNIT_BAR                1
#define LV2_PORT_UNIT_BEAT               2
#define LV2_PORT_UNIT_BPM                3
#define LV2_PORT_UNIT_CENT               4
#define LV2_PORT_UNIT_CM                 5
#define LV2_PORT_UNIT_COEF               6
#define LV2_PORT_UNIT_DB                 7
#define LV2_PORT_UNIT_DEGREE             8
#define LV2_PORT_UNIT_FRAME              9
#define LV2_PORT_UNIT_HZ                 10
#define LV2_PORT_UNIT_INCH               11
#define LV2_PORT_UNIT_KHZ                12
#define LV2_PORT_UNIT_KM                 13
#define LV2_PORT_UNIT_M                  14
#define LV2_PORT_UNIT_MHZ                15
#define LV2_PORT_UNIT_MIDINOTE           16
#define LV2_PORT_UNIT_MILE               17
#define LV2_PORT_UNIT_MIN                18
#define LV2_PORT_UNIT_MM                 19
#define LV2_PORT_UNIT_MS                 20
#define LV2_PORT_UNIT_OCT                21
#define LV2_PORT_UNIT_PC                 22
#define LV2_PORT_UNIT_S                  23
#define LV2_PORT_UNIT_SEMITONE           24

#define LV2_IS_PORT_UNIT_BAR(x)          ((x) == LV2_PORT_UNIT_BAR)
#define LV2_IS_PORT_UNIT_BEAT(x)         ((x) == LV2_PORT_UNIT_BEAT)
#define LV2_IS_PORT_UNIT_BPM(x)          ((x) == LV2_PORT_UNIT_BPM)
#define LV2_IS_PORT_UNIT_CENT(x)         ((x) == LV2_PORT_UNIT_CENT)
#define LV2_IS_PORT_UNIT_CM(x)           ((x) == LV2_PORT_UNIT_CM)
#define LV2_IS_PORT_UNIT_COEF(x)         ((x) == LV2_PORT_UNIT_COEF)
#define LV2_IS_PORT_UNIT_DB(x)           ((x) == LV2_PORT_UNIT_DB)
#define LV2_IS_PORT_UNIT_DEGREE(x)       ((x) == LV2_PORT_UNIT_DEGREE)
#define LV2_IS_PORT_UNIT_FRAME(x)        ((x) == LV2_PORT_UNIT_FRAME)
#define LV2_IS_PORT_UNIT_HZ(x)           ((x) == LV2_PORT_UNIT_HZ)
#define LV2_IS_PORT_UNIT_INCH(x)         ((x) == LV2_PORT_UNIT_INCH)
#define LV2_IS_PORT_UNIT_KHZ(x)          ((x) == LV2_PORT_UNIT_KHZ)
#define LV2_IS_PORT_UNIT_KM(x)           ((x) == LV2_PORT_UNIT_KM)
#define LV2_IS_PORT_UNIT_M(x)            ((x) == LV2_PORT_UNIT_M)
#define LV2_IS_PORT_UNIT_MHZ(x)          ((x) == LV2_PORT_UNIT_MHZ)
#define LV2_IS_PORT_UNIT_MIDINOTE(x)     ((x) == LV2_PORT_UNIT_MIDINOTE)
#define LV2_IS_PORT_UNIT_MILE(x)         ((x) == LV2_PORT_UNIT_MILE)
#define LV2_IS_PORT_UNIT_MIN(x)          ((x) == LV2_PORT_UNIT_MIN)
#define LV2_IS_PORT_UNIT_MM(x)           ((x) == LV2_PORT_UNIT_MM)
#define LV2_IS_PORT_UNIT_MS(x)           ((x) == LV2_PORT_UNIT_MS)
#define LV2_IS_PORT_UNIT_OCT(x)          ((x) == LV2_PORT_UNIT_OCT)
#define LV2_IS_PORT_UNIT_PC(x)           ((x) == LV2_PORT_UNIT_PC)
#define LV2_IS_PORT_UNIT_S(x)            ((x) == LV2_PORT_UNIT_S)
#define LV2_IS_PORT_UNIT_SEMITONE(x)     ((x) == LV2_PORT_UNIT_SEMITONE)

// Port Types
#define LV2_PORT_INPUT                   0x001
#define LV2_PORT_OUTPUT                  0x002
#define LV2_PORT_CONTROL                 0x004
#define LV2_PORT_AUDIO                   0x008
#define LV2_PORT_CV                      0x010
#define LV2_PORT_ATOM                    0x020
#define LV2_PORT_ATOM_SEQUENCE          (0x040 | LV2_PORT_ATOM)
#define LV2_PORT_EVENT                   0x080
#define LV2_PORT_MIDI_LL                 0x100

// Port Data Types
#define LV2_PORT_DATA_MIDI_EVENT         0x1000
#define LV2_PORT_DATA_OSC_EVENT          0x2000
#define LV2_PORT_DATA_PATCH_MESSAGE      0x4000
#define LV2_PORT_DATA_TIME_POSITION      0x8000

#define LV2_IS_PORT_INPUT(x)             ((x) & LV2_PORT_INPUT)
#define LV2_IS_PORT_OUTPUT(x)            ((x) & LV2_PORT_OUTPUT)
#define LV2_IS_PORT_CONTROL(x)           ((x) & LV2_PORT_CONTROL)
#define LV2_IS_PORT_AUDIO(x)             ((x) & LV2_PORT_AUDIO)
#define LV2_IS_PORT_CV(x)                ((x) & LV2_PORT_CV)
#define LV2_IS_PORT_ATOM_SEQUENCE(x)     ((x) & LV2_PORT_ATOM_SEQUENCE)
#define LV2_IS_PORT_EVENT(x)             ((x) & LV2_PORT_EVENT)
#define LV2_IS_PORT_MIDI_LL(x)           ((x) & LV2_PORT_MIDI_LL)

#define LV2_PORT_SUPPORTS_MIDI_EVENT(x)    ((x) & LV2_PORT_DATA_MIDI_EVENT)
#define LV2_PORT_SUPPORTS_OSC_EVENT(x)     ((x) & LV2_PORT_DATA_OSC_EVENT)
#define LV2_PORT_SUPPORTS_PATCH_MESSAGE(x) ((x) & LV2_PORT_DATA_PATCH_MESSAGE)
#define LV2_PORT_SUPPORTS_TIME_POSITION(x) ((x) & LV2_PORT_DATA_TIME_POSITION)

// Port Properties
#define LV2_PORT_OPTIONAL                0x0001
#define LV2_PORT_ENUMERATION             0x0002
#define LV2_PORT_INTEGER                 0x0004
#define LV2_PORT_SAMPLE_RATE             0x0008
#define LV2_PORT_TOGGLED                 0x0010
#define LV2_PORT_CAUSES_ARTIFACTS        0x0020
#define LV2_PORT_CONTINUOUS_CV           0x0040
#define LV2_PORT_DISCRETE_CV             0x0080
#define LV2_PORT_EXPENSIVE               0x0100
#define LV2_PORT_STRICT_BOUNDS           0x0200
#define LV2_PORT_LOGARITHMIC             0x0400
#define LV2_PORT_NOT_AUTOMATIC           0x0800
#define LV2_PORT_NOT_ON_GUI              0x1000
#define LV2_PORT_TRIGGER                 0x2000
#define LV2_PORT_NON_AUTOMABLE           0x4000

#define LV2_IS_PORT_OPTIONAL(x)          ((x) & LV2_PORT_OPTIONAL)
#define LV2_IS_PORT_ENUMERATION(x)       ((x) & LV2_PORT_ENUMERATION)
#define LV2_IS_PORT_INTEGER(x)           ((x) & LV2_PORT_INTEGER)
#define LV2_IS_PORT_SAMPLE_RATE(x)       ((x) & LV2_PORT_SAMPLE_RATE)
#define LV2_IS_PORT_TOGGLED(x)           ((x) & LV2_PORT_TOGGLED)
#define LV2_IS_PORT_CAUSES_ARTIFACTS(x)  ((x) & LV2_PORT_CAUSES_ARTIFACTS)
#define LV2_IS_PORT_CONTINUOUS_CV(x)     ((x) & LV2_PORT_CONTINUOUS_CV)
#define LV2_IS_PORT_DISCRETE_CV(x)       ((x) & LV2_PORT_DISCRETE_CV)
#define LV2_IS_PORT_EXPENSIVE(x)         ((x) & LV2_PORT_EXPENSIVE)
#define LV2_IS_PORT_STRICT_BOUNDS(x)     ((x) & LV2_PORT_STRICT_BOUNDS)
#define LV2_IS_PORT_LOGARITHMIC(x)       ((x) & LV2_PORT_LOGARITHMIC)
#define LV2_IS_PORT_NOT_AUTOMATIC(x)     ((x) & LV2_PORT_NOT_AUTOMATIC)
#define LV2_IS_PORT_NOT_ON_GUI(x)        ((x) & LV2_PORT_NOT_ON_GUI)
#define LV2_IS_PORT_TRIGGER(x)           ((x) & LV2_PORT_TRIGGER)
#define LV2_IS_PORT_NON_AUTOMABLE(x)     ((x) & LV2_PORT_NON_AUTOMABLE)

// Port Designation
#define LV2_PORT_DESIGNATION_CONTROL                 0x0001
#define LV2_PORT_DESIGNATION_FREEWHEELING            0x0002
#define LV2_PORT_DESIGNATION_LATENCY                 0x0004
#define LV2_PORT_DESIGNATION_SAMPLE_RATE             0x0008
#define LV2_PORT_DESIGNATION_TIME_BAR                0x0010
#define LV2_PORT_DESIGNATION_TIME_BAR_BEAT           0x0020
#define LV2_PORT_DESIGNATION_TIME_BEAT               0x0030
#define LV2_PORT_DESIGNATION_TIME_BEAT_UNIT          0x0040
#define LV2_PORT_DESIGNATION_TIME_BEATS_PER_BAR      0x0080
#define LV2_PORT_DESIGNATION_TIME_BEATS_PER_MINUTE   0x0100
#define LV2_PORT_DESIGNATION_TIME_FRAME              0x0200
#define LV2_PORT_DESIGNATION_TIME_FRAMES_PER_SECOND  0x0400
#define LV2_PORT_DESIGNATION_TIME_SPEED              0x0800
#define LV2_PORT_DESIGNATION_TIME_TICKS_PER_BEAT     0x1000

#define LV2_IS_PORT_DESIGNATION_CONTROL(x)           ((x) == LV2_PORT_DESIGNATION_CONTROL)
#define LV2_IS_PORT_DESIGNATION_FREEWHEELING(x)      ((x) == LV2_PORT_DESIGNATION_FREEWHEELING)
#define LV2_IS_PORT_DESIGNATION_LATENCY(x)           ((x) == LV2_PORT_DESIGNATION_LATENCY)
#define LV2_IS_PORT_DESIGNATION_SAMPLE_RATE(x)       ((x) == LV2_PORT_DESIGNATION_SAMPLE_RATE)
#define LV2_IS_PORT_DESIGNATION_TIME_BAR(x)          ((x) == LV2_PORT_DESIGNATION_TIME_BAR)
#define LV2_IS_PORT_DESIGNATION_TIME_BAR_BEAT(x)     ((x) == LV2_PORT_DESIGNATION_TIME_BAR_BEAT)
#define LV2_IS_PORT_DESIGNATION_TIME_BEAT(x)         ((x) == LV2_PORT_DESIGNATION_TIME_BEAT)
#define LV2_IS_PORT_DESIGNATION_TIME_BEAT_UNIT(x)    ((x) == LV2_PORT_DESIGNATION_TIME_BEAT_UNIT)
#define LV2_IS_PORT_DESIGNATION_TIME_BEATS_PER_BAR(x)     ((x) == LV2_PORT_DESIGNATION_TIME_BEATS_PER_BAR)
#define LV2_IS_PORT_DESIGNATION_TIME_BEATS_PER_MINUTE(x)  ((x) == LV2_PORT_DESIGNATION_TIME_BEATS_PER_MINUTE)
#define LV2_IS_PORT_DESIGNATION_TIME_FRAME(x)             ((x) == LV2_PORT_DESIGNATION_TIME_FRAME)
#define LV2_IS_PORT_DESIGNATION_TIME_FRAMES_PER_SECOND(x) ((x) == LV2_PORT_DESIGNATION_TIME_FRAMES_PER_SECOND)
#define LV2_IS_PORT_DESIGNATION_TIME_SPEED(x)             ((x) == LV2_PORT_DESIGNATION_TIME_SPEED)
#define LV2_IS_PORT_DESIGNATION_TIME_TICKS_PER_BEAT(x)    ((x) == LV2_PORT_DESIGNATION_TIME_TICKS_PER_BEAT)
#define LV2_IS_PORT_DESIGNATION_TIME(x)                   ((x) >= LV2_PORT_DESIGNATION_TIME_BAR && (x) <= LV2_PORT_DESIGNATION_TIME_TICKS_PER_BEAT)

// Feature Types
#define LV2_FEATURE_OPTIONAL             1
#define LV2_FEATURE_REQUIRED             2

#define LV2_IS_FEATURE_OPTIONAL(x)       ((x) == LV2_FEATURE_OPTIONAL)
#define LV2_IS_FEATURE_REQUIRED(x)       ((x) == LV2_FEATURE_REQUIRED)

// // UI Types
// #define LV2_UI_GTK2                      1
// #define LV2_UI_GTK3                      2
// #define LV2_UI_QT4                       3
// #define LV2_UI_QT5                       4
// #define LV2_UI_COCOA                     5
// #define LV2_UI_WINDOWS                   6
// #define LV2_UI_X11                       7
// #define LV2_UI_EXTERNAL                  8
// #define LV2_UI_OLD_EXTERNAL              9
// #define LV2_UI_MOD                       10

// #define LV2_IS_UI_GTK2(x)                ((x) == LV2_UI_GTK2)
// #define LV2_IS_UI_GTK3(x)                ((x) == LV2_UI_GTK3)
// #define LV2_IS_UI_QT4(x)                 ((x) == LV2_UI_QT4)
// #define LV2_IS_UI_QT5(x)                 ((x) == LV2_UI_QT5)
// #define LV2_IS_UI_COCOA(x)               ((x) == LV2_UI_COCOA)
// #define LV2_IS_UI_WINDOWS(x)             ((x) == LV2_UI_WINDOWS)
// #define LV2_IS_UI_X11(x)                 ((x) == LV2_UI_X11)
// #define LV2_IS_UI_EXTERNAL(x)            ((x) == LV2_UI_EXTERNAL)
// #define LV2_IS_UI_OLD_EXTERNAL(x)        ((x) == LV2_UI_OLD_EXTERNAL)
// #define LV2_IS_UI_MOD(x)                 ((x) == LV2_UI_MOD)

// // Plugin Types
// #define LV2_PLUGIN_DELAY                 0x000001
// #define LV2_PLUGIN_REVERB                0x000002
// #define LV2_PLUGIN_SIMULATOR             0x000004
// #define LV2_PLUGIN_DISTORTION            0x000008
// #define LV2_PLUGIN_WAVESHAPER            0x000010
// #define LV2_PLUGIN_DYNAMICS              0x000020
// #define LV2_PLUGIN_AMPLIFIER             0x000040
// #define LV2_PLUGIN_COMPRESSOR            0x000080
// #define LV2_PLUGIN_ENVELOPE              0x000100
// #define LV2_PLUGIN_EXPANDER              0x000200
// #define LV2_PLUGIN_GATE                  0x000400
// #define LV2_PLUGIN_LIMITER               0x000800
// #define LV2_PLUGIN_EQ                    0x001000
// #define LV2_PLUGIN_MULTI_EQ              0x002000
// #define LV2_PLUGIN_PARA_EQ               0x004000
// #define LV2_PLUGIN_FILTER                0x008000
// #define LV2_PLUGIN_ALLPASS               0x010000
// #define LV2_PLUGIN_BANDPASS              0x020000
// #define LV2_PLUGIN_COMB                  0x040000
// #define LV2_PLUGIN_HIGHPASS              0x080000
// #define LV2_PLUGIN_LOWPASS               0x100000

// #define LV2_PLUGIN_GENERATOR             0x000001
// #define LV2_PLUGIN_CONSTANT              0x000002
// #define LV2_PLUGIN_INSTRUMENT            0x000004
// #define LV2_PLUGIN_OSCILLATOR            0x000008
// #define LV2_PLUGIN_MODULATOR             0x000010
// #define LV2_PLUGIN_CHORUS                0x000020
// #define LV2_PLUGIN_FLANGER               0x000040
// #define LV2_PLUGIN_PHASER                0x000080
// #define LV2_PLUGIN_SPATIAL               0x000100
// #define LV2_PLUGIN_SPECTRAL              0x000200
// #define LV2_PLUGIN_PITCH                 0x000400
// #define LV2_PLUGIN_UTILITY               0x000800
// #define LV2_PLUGIN_ANALYSER              0x001000
// #define LV2_PLUGIN_CONVERTER             0x002000
// #define LV2_PLUGIN_FUNCTION              0x008000
// #define LV2_PLUGIN_MIXER                 0x010000

// #define LV2_GROUP_DELAY                  (LV2_PLUGIN_DELAY|LV2_PLUGIN_REVERB)
// #define LV2_GROUP_DISTORTION             (LV2_PLUGIN_DISTORTION|LV2_PLUGIN_WAVESHAPER)
// #define LV2_GROUP_DYNAMICS               (LV2_PLUGIN_DYNAMICS|LV2_PLUGIN_AMPLIFIER|LV2_PLUGIN_COMPRESSOR|LV2_PLUGIN_ENVELOPE|LV2_PLUGIN_EXPANDER|LV2_PLUGIN_GATE|LV2_PLUGIN_LIMITER)
// #define LV2_GROUP_EQ                     (LV2_PLUGIN_EQ|LV2_PLUGIN_MULTI_EQ|LV2_PLUGIN_PARA_EQ)
// #define LV2_GROUP_FILTER                 (LV2_PLUGIN_FILTER|LV2_PLUGIN_ALLPASS|LV2_PLUGIN_BANDPASS|LV2_PLUGIN_COMB|LV2_GROUP_EQ|LV2_PLUGIN_HIGHPASS|LV2_PLUGIN_LOWPASS)
// #define LV2_GROUP_GENERATOR              (LV2_PLUGIN_GENERATOR|LV2_PLUGIN_CONSTANT|LV2_PLUGIN_INSTRUMENT|LV2_PLUGIN_OSCILLATOR)
// #define LV2_GROUP_MODULATOR              (LV2_PLUGIN_MODULATOR|LV2_PLUGIN_CHORUS|LV2_PLUGIN_FLANGER|LV2_PLUGIN_PHASER)
// #define LV2_GROUP_REVERB                 (LV2_PLUGIN_REVERB)
// #define LV2_GROUP_SIMULATOR              (LV2_PLUGIN_SIMULATOR|LV2_PLUGIN_REVERB)
// #define LV2_GROUP_SPATIAL                (LV2_PLUGIN_SPATIAL)
// #define LV2_GROUP_SPECTRAL               (LV2_PLUGIN_SPECTRAL|LV2_PLUGIN_PITCH)
// #define LV2_GROUP_UTILITY                (LV2_PLUGIN_UTILITY|LV2_PLUGIN_ANALYSER|LV2_PLUGIN_CONVERTER|LV2_PLUGIN_FUNCTION|LV2_PLUGIN_MIXER)

// #define LV2_IS_DELAY(x, y)               ((x) & LV2_GROUP_DELAY)
// #define LV2_IS_DISTORTION(x, y)          ((x) & LV2_GROUP_DISTORTION)
// #define LV2_IS_DYNAMICS(x, y)            ((x) & LV2_GROUP_DYNAMICS)
// #define LV2_IS_EQ(x, y)                  ((x) & LV2_GROUP_EQ)
// #define LV2_IS_FILTER(x, y)              ((x) & LV2_GROUP_FILTER)
// #define LV2_IS_GENERATOR(x, y)           ((y) & LV2_GROUP_GENERATOR)
// #define LV2_IS_INSTRUMENT(x, y)          ((y) & LV2_PLUGIN_INSTRUMENT)
// #define LV2_IS_MODULATOR(x, y)           ((y) & LV2_GROUP_MODULATOR)
// #define LV2_IS_REVERB(x, y)              ((x) & LV2_GROUP_REVERB)
// #define LV2_IS_SIMULATOR(x, y)           ((x) & LV2_GROUP_SIMULATOR)
// #define LV2_IS_SPATIAL(x, y)             ((y) & LV2_GROUP_SPATIAL)
// #define LV2_IS_SPECTRAL(x, y)            ((y) & LV2_GROUP_SPECTRAL)
// #define LV2_IS_UTILITY(x, y)             ((y) & LV2_GROUP_UTILITY)


#define LV2_KXSTUDIO_PROPERTIES__NonAutomable             LV2_KXSTUDIO_PROPERTIES_PREFIX "NonAutomable"
#define LV2_KXSTUDIO_PROPERTIES__TimePositionTicksPerBeat LV2_KXSTUDIO_PROPERTIES_PREFIX "TimePositionTicksPerBeat"
#define LV2_KXSTUDIO_PROPERTIES__TransientWindowId        LV2_KXSTUDIO_PROPERTIES_PREFIX "TransientWindowId"

#define LV2_EXTERNAL_UI_URI     "http://kxstudio.sf.net/ns/lv2ext/external-ui"
#define LV2_EXTERNAL_UI_PREFIX  LV2_EXTERNAL_UI_URI "#"
#define LV2_EXTERNAL_UI__Host   LV2_EXTERNAL_UI_PREFIX "Host"
#define LV2_EXTERNAL_UI__Widget LV2_EXTERNAL_UI_PREFIX "Widget"
/** This extension used to be defined by a lv2plug.in URI */
#define LV2_EXTERNAL_UI_DEPRECATED_URI "http://lv2plug.in/ns/extensions/ui#external"


// -----------------------------------------------------------------------
// Define namespaces and missing prefixes

#define NS_dct  "http://purl.org/dc/terms/"
#define NS_doap "http://usefulinc.com/ns/doap#"
#define NS_rdf  "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_rdfs "http://www.w3.org/2000/01/rdf-schema#"
#define NS_llmm "http://ll-plugins.nongnu.org/lv2/ext/midimap#"
#define NS_mod  "http://portalmod.com/ns/modgui#"

#define LV2_MIDI_Map__CC      "http://ll-plugins.nongnu.org/lv2/namespace#CC"
#define LV2_MIDI_Map__NRPN    "http://ll-plugins.nongnu.org/lv2/namespace#NRPN"

#define LV2_MIDI_LL__MidiPort "http://ll-plugins.nongnu.org/lv2/ext/MidiPort"

#define LV2_UI__Qt5UI         LV2_UI_PREFIX "Qt5UI"
#define LV2_UI__makeResident  LV2_UI_PREFIX "makeResident"

//-----------------------------------------------------------------------

//      MIDI CODES
// cmd	Meaning	               # params  param 1        param 2
//0x80	Note-off                2        key	        velocity
//0x90	Note-on                 2        key	        velocity
//0xA0	Aftertouch              2        key	        touch
//0xB0	Continuous controller   2        controller #	controller value
//0xC0	Patch change            2        instrument #
//0xD0	Channel Pressure        1        pressure
//0xE0	Pitch bend              2        lsb (7 bits)   msb (7 bits)

//0xF0 commands = System messages
//    A basic table of system messages

// cmd	meaning	# param
//0xF0	start of system exclusive message variable
//0xF1	MIDI Time Code Quarter Frame (Sys Common)
//0xF2	Song Position Pointer (Sys Common)
//0xF3	Song Select (Sys Common)
//0xF4	???
//0xF5	???
//0xF6	Tune Request (Sys Common)
//0xF7	end of system exclusive message	0
//0xF8	Timing Clock (Sys Realtime)
//0xFA	Start (Sys Realtime)
//0xFB	Continue (Sys Realtime)
//0xFC	Stop (Sys Realtime)
//0xFD	???
//0xFE	Active Sensing (Sys Realtime)
//0xFF	System Reset (Sys Realtime)
