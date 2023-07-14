#pragma once


#include <string>
#include <boost/dll.hpp>

#include <gtk/gtk.h>


// #if GDK_WINDOWING_WIN32

// #include <gdk/win32/gdkwin32.h>
// #define WIN_ID_TYPE HWND
// #define WIN_ID_FUNC(widget) GDK_WINDOW_HWND(gtk_widget_get_window(widget));

// #elif GDK_WINDOWING_QUARTZ

// #include <gdk/quartz/gdkquartz.h>
// #define WIN_ID_TYPE widget
// #define WIN_ID_FUNC(WIDGET) gdk_quartz_window_get_nsview(gtk_widget_get_window(widget));

// #else // GDK_WINDOWING_X11

#include <gdk/gdkx.h>

//


#define WIN_ID_TYPE gulong
#define WIN_ID_FUNC(widget) gdk_x11_window_get_xid(gtk_widget_get_window(widget))








//std::string get_plugin_string(AEffect* plugin, VstInt32 opcode, int index);

//std::string get_param_name(AEffect* plugin, int index);

//VstParameterProperties* get_param_props(AEffect* plugin, int index);

//AEffect* load_vst(boost::dll::shared_library& lib, std::string vst_filename, AEffectDispatcherProc callback, void* user_p);

//typedef VstIntPtr (*DispatcherProc) (AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);


extern "C" {

    // only used by VstPlugins class when searching for plugins in the vst folders. plugins tend to ask what version of vst the host supports (opcode 1).
//    VstIntPtr VSTCALLBACK dummyHostCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);


}

//inline VstInt32 dispatch(AEffect* plugin, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt) {
//    return ((AEffectDispatcherProc) plugin->dispatcher)(plugin, opcode, index, value, ptr, opt);
//}

//inline VstInt32 dispatch(AEffect* plugin, VstInt32 opcode) {
//    return dispatch(plugin, opcode, 0, 0, 0, 0);
//}



//inline VstMidiEvent* vst_midi_event(std::array<uint8_t, 3> data) {
//    auto event = new VstMidiEvent();
//    memset(event, 0, sizeof(VstMidiEvent));
//    event->type = kVstMidiType;
//    event->byteSize = sizeof(VstMidiEvent);
//    event->deltaFrames = 0;
//    event->noteLength = 0;
//    event->noteOffset = 0;
//    memcpy(event->midiData, &data[0], 3);
//    event->detune = 0;
//    event->noteOffVelocity = 0;

//    return event;
//}

//inline VstMidiEvent* midi_note_on(uint8_t note, uint8_t volume) {
//    return vst_midi_event({MIDI_MSG_NOTE_ON, MIDI_NOTE(note), volume});
//}


//inline VstMidiEvent* midi_note_off(uint8_t note) {
//    return vst_midi_event({MIDI_MSG_NOTE_OFF, MIDI_NOTE(note), 0});
//}

//inline VstMidiEvent* midi_note_aftertouch(uint8_t note, uint8_t volume) {
//    return vst_midi_event({MIDI_MSG_NOTE_PRESSURE, MIDI_NOTE(note), volume});
//}
