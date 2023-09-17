#pragma once

#include <iostream>
#include <vector>
#include <functional>

#include <public.sdk/source/vst/hosting/processdata.h>
#include <public.sdk/source/vst/hosting/eventlist.h>
#include <pluginterfaces/vst/ivstprocesscontext.h>
#include <pluginterfaces/vst/ivstcomponent.h>
#include <pluginterfaces/vst/ivstevents.h>
#include <pluginterfaces/gui/iplugview.h>

#include "zzub/plugin.h"

#include "libzzub/midi_track.h"

#include "Info.h"
#include "Defines.h"

extern "C" {
    void on_window_destroy(GtkWidget* widget, gpointer data);
}

struct BusSummary {
    uint16_t bus_count = 0;
    int16_t main_bus_index = 0;
    uint16_t main_channel_count = 0;
};

struct BusSummaries {
    BusSummary in;
    BusSummary out;
};

struct WindowResizer: Steinberg::IPlugFrame {
    WindowResizer(std::function<bool(int, int)> resizer): resizer(resizer) {}

    // implement IPlugFrame methods
    virtual Steinberg::tresult PLUGIN_API resizeView(Steinberg::IPlugView* view, Steinberg::ViewRect* newSize) {
        if(!view || !newSize)
            return Steinberg::kInvalidArgument;

        if(!resizer(newSize->getWidth(), newSize->getHeight()))
            return Steinberg::kResultFalse;

        view->onSize(newSize);

        return Steinberg::kResultOk;
    }

    virtual Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID iid, void** obj) { 
        if (iid == Steinberg::FUnknown::iid || iid == Steinberg::IPlugFrame::iid) 
            return Steinberg::kResultOk; 
        else
            return Steinberg::kNoInterface;
    };

    // fake FUnknown methods. this interface should be one method called resizeView() and I resent having to pretend to be a COM object
    virtual Steinberg::uint32 PLUGIN_API addRef() { return 1; };
    virtual Steinberg::uint32 PLUGIN_API release() { return 1; };

private:
    std::function<bool(int, int)> resizer;
};


struct Vst3PluginAdapter: zzub::plugin, zzub::midi_plugin_interface, zzub::event_handler {
    Vst3PluginAdapter(
        const zzub::info* info, 
        Steinberg::Vst::PlugProvider* provider
    );

    virtual ~Vst3PluginAdapter();


    virtual void init(zzub::archive* pi) override;    
    virtual void created() override;
    virtual bool invoke(zzub_event_data_t& data) override;
    virtual void process_events() override;
    virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode) override;
    virtual void set_track_count(int count) override;


    // midi_plugin_interface methods
    virtual void add_note_on(uint8_t note, uint8_t volume) override;
    virtual void add_note_off(uint8_t note) override;
    virtual void add_aftertouch(uint8_t note, uint8_t volume) override;
    virtual void add_midi_command(uint8_t cmd, uint8_t data1, uint8_t data2) override;
    virtual zzub::midi_note_track *get_track_data_pointer(uint16_t track_num) const override;

    void ui_open();
    void ui_close();

    // rescale plugin content and resize gtk window
    bool ui_rescale(float factor);
    bool ui_resize(int width, int height);

    bool is_ok() const {
        return ok;
    }


    template <typename T>
    void add_midi_event(int32_t bus, int32_t sampleOffset, double qtr_notes, uint16_t event_type, T event_data, bool is_live) {
        if(bus < 0 || bus > event_buses.in.bus_count)
            return;

        uint16_t flags = is_live ? Steinberg::Vst::Event::kIsLive : 0;

        auto event = Steinberg::Vst::Event { bus, sampleOffset, qtr_notes, flags, event_type, {} };

        // first tried creating an Event like this: Steinberg::Vst::Event { bus, sampleOffset, qtr_notes, flags, event_type, event_data };
        // c++ was trying to create a NoteOnEvent union and initialise the int16 channel member of the NoteOnEvent with event_data
        // this is the workaround
        memcpy(&event.data, &event_data, sizeof(T));

        process_data.inputEvents[0].addEvent(event);
    }

private:
    Steinberg::Vst::AudioBusBuffers* init_audio_buffers(Steinberg::Vst::BusDirections direction, BusSummary& main_bus);
    Steinberg::Vst::EventList* init_event_buffers(Steinberg::Vst::BusDirections direction, BusSummary& main_bus);

    bool ok = false;
    bool editor_is_open = false;
    const Vst3Info* info = nullptr;
    zzub::midi_track_manager midi_track_manager;

    Steinberg::Vst::ProcessSetup process_setup = {};
    Steinberg::Vst::HostProcessData process_data = {};
	Steinberg::Vst::ProcessContext process_context = {};

    Steinberg::IPtr<Steinberg::Vst::IEditController> controller ;
    Steinberg::IPtr<Steinberg::Vst::IComponent> component ;
    Steinberg::IPtr<Steinberg::Vst::IAudioProcessor> processor ;
    Steinberg::IPtr<Steinberg::IPlugView> plugin_view ;
    Steinberg::Vst::PlugProvider* provider = nullptr;

    std::vector<Steinberg::Vst::Event> eventbuf{};

    BusSummaries audio_buses;
    BusSummaries event_buses;

    WindowResizer window_resizer;
    GtkWidget* window = nullptr;
    
    uint16_t* globals = nullptr;
    uint16_t num_tracks = 0;
    zzub_plugin_t* metaplugin = nullptr;
    float ui_content_scale = 1.0f;
};