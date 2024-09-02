#include "vst3_adapter.h"

#include "public.sdk/source/vst/hosting/eventlist.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"


namespace Vst = Steinberg::Vst;


extern "C" {
    void on_window_destroy(GtkWidget* widget, gpointer data) {
        static_cast<Vst3PluginAdapter*>(data)->ui_close();
    }
}


Vst3PluginAdapter::Vst3PluginAdapter(
    const zzub::info* info,
    Vst::PlugProvider* provider 
) : info((const Vst3Info*) info),
    provider(provider), 
    midi_track_manager(*this, info->max_tracks),
    window_resizer([this](int width, int height) { return ui_resize(width, height); })
{
    printf("build vst3 plugin\n");

    if(info->flags & zzub_plugin_flag_has_midi_input) {
        track_values = midi_track_manager.get_track_data();
        num_tracks = 1;
        set_track_count(num_tracks);
    }
}


Vst3PluginAdapter::~Vst3PluginAdapter() {
    delete provider;
    // delete audio bus buffers
                        
    for (auto idx = 0; idx < info->get_bus_count(Vst::MediaTypes::kAudio, Vst::BusDirections::kInput); idx++) {
        free(process_data.inputs[idx].channelBuffers32);
    }

    for (auto idx = 0; idx < info->get_bus_count(Vst::MediaTypes::kAudio, Vst::BusDirections::kOutput); idx++) {
        free(process_data.outputs[idx].channelBuffers32);
    }

    free(process_data.inputs);
    free(process_data.outputs);
    
    free(copy_in);
    free(copy_out);
}




void Vst3PluginAdapter::init(zzub::archive* pi) 
{
    metaplugin = _host->get_metaplugin();
    _host->set_event_handler(metaplugin, this);

    auto sample_size = Vst::kSample32;
    // auto meta_plugin = _host->get_metaplugin();
    auto max_block_size = zzub::buffer_size;
    auto realtime = true;


    if(info->get_global_param_count() > 0) {
        global_values = globals = (uint16_t*) malloc(sizeof(uint16_t) * info->get_global_param_count());
    }


    if(info->flags & zzub_plugin_flag_has_midi_input) {
        midi_track_manager.init(_master_info->samples_per_second);
    }

    // _host->set_event_handler(meta_plugin, this);

	process_setup.processMode = realtime;
	process_setup.symbolicSampleSize = sample_size;
	process_setup.sampleRate = _master_info->samples_per_second;
	process_setup.maxSamplesPerBlock = max_block_size;

	process_data.numSamples = 0;
	process_data.symbolicSampleSize = sample_size;
	process_data.processContext = &process_context;
    
    process_context.sampleRate = _master_info->samples_per_second;
    process_context.projectTimeSamples = 0;

    component = provider->getComponent();
    processor = Steinberg::FUnknownPtr<Vst::IAudioProcessor>(component);
    
    if(component->initialize(&host_context) != Steinberg::kResultOk) {
          printf("component %s failed to initialize\n", info->name.c_str());  
    } else {
        printf("component %s initialized\n", info->name.c_str());

    }

    controller = provider->getController();

    if(controller && controller->initialize(&host_context) != Steinberg::kResultOk) {
        printf("controller %s failed to initialize\n", info->name.c_str());  
        return;
    } else {
        printf("controller %s initialized\n", info->name.c_str());
    }


    // if(controller && component) {
    //     Steinberg::FUnknownPtr<Vst::IConnectionPoint> component_p(component);
    //     Steinberg::FUn   knownPtr<Vst::IConnectionPoint> controller_p(controller);

    //     if (component_p && controller_p) {
    //         printf("Connecting controller and component\n");
    //         component_p->connect(controller_p);
    //         controller_p->connect(component_p);
    //     }
    // }

    initialised = true;
}




std::vector<Vst::SpeakerArrangement> 
Vst3PluginAdapter::get_actual_arrangements(
    Steinberg::Vst::BusDirections direction
) {
    std::vector<Vst::SpeakerArrangement> speakers;
    Vst::SpeakerArrangement arrangement;

    for(auto idx=0; idx < info->get_bus_count(Vst::MediaTypes::kAudio, direction); idx++) {
        processor->getBusArrangement(direction, idx, arrangement);
        speakers.push_back(arrangement);
    }

    return speakers;
}




bool 
Vst3PluginAdapter::setup_speaker_arrangements() 
{
    auto audio_in_count = info->get_bus_count(Vst::MediaTypes::kAudio, Vst::BusDirections::kInput);
    auto audio_out_count = info->get_bus_count(Vst::MediaTypes::kAudio, Vst::BusDirections::kOutput);

    std::vector<Vst::SpeakerArrangement> input_speakers(audio_in_count, Vst::SpeakerArr::kStereo);
    std::vector<Vst::SpeakerArrangement> output_speakers(audio_out_count, Vst::SpeakerArr::kStereo);

    auto res = processor->setBusArrangements(
        input_speakers.data(), audio_in_count,
        output_speakers.data(), audio_out_count
    );

    if(res != Steinberg::kResultOk) {
        auto actual_ins = get_actual_arrangements(Vst::BusDirections::kInput);
        auto actual_outs = get_actual_arrangements(Vst::BusDirections::kOutput);
        
        if(actual_ins.size() != audio_in_count || actual_outs.size() != audio_out_count) {
            printf("failed to set audio bus arrangements for '%s', bus count mismatch.\n", info->name.c_str());
            return false;
        }

        if(actual_ins != input_speakers || actual_outs != output_speakers) {
            printf("failed to set audio bus arrangements for '%s', speaker arrangement problem\n", info->name.c_str());
            return false;
        }       
    }

    return true;
}




void 
Vst3PluginAdapter::created() 
{
    if(!setup_speaker_arrangements()) {
        return;
    }

    auto res = processor->setupProcessing(process_setup);
    if (res != Steinberg::kResultOk) {
        std::cout << "Failed to setup VST processing: " << res << std::endl;
        return;
    }



    process_data.prepare(*component, process_setup.maxSamplesPerBlock, process_setup.symbolicSampleSize);

    process_data.inputParameterChanges = new Vst::ParameterChanges();

    process_data.inputs = init_audio_buffers(Vst::BusDirections::kInput, audio_buses.in);
    process_data.outputs = init_audio_buffers(Vst::BusDirections::kOutput, audio_buses.out);

    // find main audio bus and number of channels in audio bus
    process_data.inputEvents = init_event_buffers(Vst::BusDirections::kInput, event_buses.in);
    process_data.outputEvents = init_event_buffers(Vst::BusDirections::kOutput, event_buses.out);

    copy_in = zzub::tools::CopyChannels::build(2, audio_buses.in.main_channel_count);
    copy_out = zzub::tools::CopyChannels::build(audio_buses.out.main_channel_count, 2);

    if (component->setActive(true) != Steinberg::kResultTrue) {
        std::cout << "Failed to activate VST component" << std::endl;
        return;
    }

    res = processor->setProcessing(true);
    ok = true;
}



bool Vst3PluginAdapter::invoke(zzub_event_data_t& data) 
{
    if(data.type == zzub::event_type_double_click && !plugin_view && is_ok()) {
        ui_open();
    }

    // if (!plugin || data.type != zzub::event_type_double_click || is_editor_open || !(info->flags & zzub_plugin_flag_has_custom_gui)) {
    return false;
}




void Vst3PluginAdapter::ui_open() 
{
    plugin_view = controller->createView(Vst::ViewType::kEditor);
    if (!plugin_view) {
        std::cout << "Failed to create plugin view" << std::endl;
        return;
    }
    
    if (plugin_view->isPlatformTypeSupported(Steinberg::kPlatformTypeX11EmbedWindowID) != Steinberg::kResultTrue) {
		std::cout << "Editor view does not support X11" << std::endl;
		return;
    }

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), this);

    gtk_window_set_title(GTK_WINDOW(window), info->name.c_str());
    gtk_window_present(GTK_WINDOW(window));

    auto win_id = WIN_ID_FUNC(window);

    if (plugin_view->attached((void*) win_id, Steinberg::kPlatformTypeX11EmbedWindowID) != Steinberg::kResultOk) {
        std::cout << "Failed to attach plugin view" << std::endl;
        return;
    }

    // make the plugin rescale the ui 
    ui_content_scale = gtk_widget_get_scale_factor((GtkWidget*)_host->get_host_info()->host_ptr);

    ui_rescale(ui_content_scale);

    plugin_view->setFrame(&window_resizer);
}




// this makes the plugin rescale the interface - if possible - then resize the gtk window to match
bool Vst3PluginAdapter::ui_rescale(float factor) 
{
    if (!plugin_view || !window) {
        return false;
    }

    auto plugin_scale_iface = Steinberg::FUnknownPtr<Steinberg::IPlugViewContentScaleSupport>(plugin_view);
    if(plugin_scale_iface) {
        plugin_scale_iface->setContentScaleFactor(ui_content_scale);
    }

    Steinberg::ViewRect view_rect = {};
	if (plugin_view->getSize(&view_rect) == Steinberg::kResultOk) {
        ui_resize(view_rect.getWidth(), view_rect.getHeight());
	}

    return true;
}




bool 
Vst3PluginAdapter::ui_resize(int width, int height) 
{
    if(!window)
        return false;

    gtk_window_resize(GTK_WINDOW(window), width, height);
    return true;
}



void 
Vst3PluginAdapter::ui_close() 
{
    if (!plugin_view) {
        return;
    }

    plugin_view->removed();
    plugin_view = nullptr;

    gtk_widget_destroy(window);
    window = nullptr;
}





/**
 * @brief 
 * 
 * @param bus_buffers either process_data.inputs or process_data.outputs 
 * @param direction kInput or kOutput
 * @param MainAudioBusInfo& main_bus Will store the index of the main bus and number of channels it has
 */

Vst::AudioBusBuffers* 
    Vst3PluginAdapter::init_audio_buffers(
    Vst::BusDirections direction, 
    BusSummary& bus_summary
) {
    auto bus_count = bus_summary.bus_count = info->get_bus_count(Vst::MediaTypes::kAudio, direction);
    auto buffers = new Vst::AudioBusBuffers[bus_count];

    auto bus_infos = info->get_bus_infos(Vst::MediaTypes::kAudio, direction);

    for (auto idx = 0; idx < bus_count; idx++) {
        buffers[idx].numChannels = bus_infos[idx].channelCount;
        buffers[idx].silenceFlags = 0;
        buffers[idx].channelBuffers32 = (float**) malloc(sizeof(float*) * bus_infos[idx].channelCount);

        for(auto chan_idx = 0; chan_idx < bus_infos[idx].channelCount; chan_idx++) {
            float* audio_buf = (float*) malloc(sizeof(float) * zzub::buffer_size);
            memset(audio_buf, 0, sizeof(float) * zzub::buffer_size);
            buffers[idx].channelBuffers32[chan_idx] = audio_buf;
        }

        if(bus_infos[idx].busType == Vst::BusTypes::kMain) {
            bus_summary.main_bus_index = idx;
            bus_summary.main_channel_count = bus_infos[idx].channelCount;
        }

        auto res = component->activateBus(Vst::MediaTypes::kAudio, direction, idx, true);
    }

    return buffers;
}




/**
 * @brief 
 * 
 * @param bus_buffers either process_data.inputs or process_data.outputs 
 * @param direction kInput or kOutput
 * @param MainAudioBusInfo& main_bus Will store the index of the main bus and number of channels it has
 */

Vst::EventList* 
Vst3PluginAdapter::init_event_buffers(
    Vst::BusDirections direction, 
    BusSummary& bus_summary
) {
    auto bus_count = bus_summary.bus_count = info->get_bus_count(Vst::MediaTypes::kEvent, direction);
    auto events = new Vst::EventList[bus_count];

    auto bus_infos = info->get_bus_infos(Vst::MediaTypes::kEvent, direction);

    for (auto idx = 0; idx < bus_count; idx++) {
        if(bus_infos[idx].busType == Vst::BusTypes::kMain) {
            bus_summary.main_bus_index = idx;
        }

        component->activateBus(Vst::MediaTypes::kEvent, direction, idx, true);
    }

    if(bus_summary.main_bus_index == -1) {
        bus_summary.main_bus_index = 0;
    }

    bus_summary.main_channel_count = 1;

    return events;
}




void Vst3PluginAdapter::set_track_count(
    int track_count
) {
    midi_track_manager.set_track_count(track_count);
    num_tracks = track_count;
}




void Vst3PluginAdapter::process_events() 
{
    if (!ok)
        return;

    for (auto idx = 0; idx < info->get_global_param_count(); idx++) {
        auto vst_param = info->get_vst_param(idx);
        uint16_t value = globals[idx];

        if (value == vst_param->zzub_param.value_none)
            continue;

        int32_t pd_index, pq_index = 0;
        auto* param_queue = process_data.inputParameterChanges->addParameterData(vst_param->param_id, pd_index);
        if (param_queue == nullptr) {
            std::cout << "Failed to add parameter data" << std::endl;
            continue;
        }

        param_queue->addPoint(0, vst_param->to_vst_value(value), pq_index);
    }

    if (info->flags & zzub_plugin_flag_has_midi_input)
        midi_track_manager.process_events();

    // if(changes->getParameterCount() > 0) {
    //     auto res = controller->performEdit(changes);
    //     if (res != Steinberg::kResultOk) {
    //         std::cout << "Failed to perform edit" << std::endl;
    //     }
    // }

    // for (auto idx = 0; idx < info->get_num_buses(MediaTypes::kEvent, BusDirections::kInput); idx++) {
    //     auto* event_list = process_data.inputEvents[idx];
    //     auto* midi_track = (struct zzub::midi_note_track*) track_values[idx];

    //     if (midi_track == nullptr)
    //         continue;

    //     for (auto idx = 0; idx < midi_track->num_events; idx++) {
    //         auto* event = event_list->addEvent();
    //         event->busIndex = 0;
    //         event->sampleOffset = midi_track->events[idx].sample_offset;
    //         event->flags = Vst::Event::kIsLive;
    //         event->type = Vst::Event::kNoteOnEvent;
    //         event->noteOn.pitch = midi_track->events[idx].pitch;
    //         event->noteOn.velocity = midi_track->events[idx].velocity;
    //         event->noteOn.length = midi_track->events[idx].length;
    //     }
    // }

    // if (info->flags & zzub_plugin_flag_has_midi_input)
        // midi_track_manager.process_events();
}




void Vst3PluginAdapter::add_note_on(
    uint8_t note, 
    uint8_t volume
) {
    printf("add note on: %d, %d event count %d\n", note, volume, process_data.inputEvents[0].getEventCount());
    add_midi_event(0, 0, 0.0, Vst::Event::kNoteOnEvent, Vst::NoteOnEvent{0, note, 0.0f, volume / 127.0f, 10000, -1}, false);
}




void Vst3PluginAdapter::add_note_off(
    uint8_t note
) {
    add_midi_event(0, 0, 0.0, Vst::Event::kNoteOffEvent, Vst::NoteOffEvent{0, note, 0.0f, -1, 0.0f}, false);
}




void Vst3PluginAdapter::add_aftertouch(
    uint8_t note, 
    uint8_t volume
) {
    add_midi_event(0, 0, 0.0, Vst::Event::kPolyPressureEvent, Vst::PolyPressureEvent{0, note, volume / 127.0f, -1}, false);
}




void Vst3PluginAdapter::add_midi_command(
    uint8_t cmd, 
    uint8_t data1, 
    uint8_t data2
) {
    
}


bool Vst3PluginAdapter::process_stereo(
    float **pin, 
    float **pout, 
    int numsamples, 
    int mode
) {
    if (info->flags & zzub_plugin_flag_has_midi_input) {
        midi_track_manager.process_samples(numsamples, mode);

        if (eventbuf.size() > 0) {
            printf("vst_adapter sed midi events\n");
            // vst_events->numEvents = midi_events.size();
            // memcpy(&vst_events->events[0], &midi_events[0], vst_events->numEvents * sizeof(VstMidiEvent*));
            // dispatch(plugin, effProcessEvents, 0, 0, vst_events, 0.f);
            // clear_vst_events();
        }
    }

    if (mode == zzub::process_mode_no_io)
        return 1;

    copy_in->copy(pin, process_data.inputs[0].channelBuffers32, numsamples);

    process_data.numSamples = numsamples;
    auto res = processor->process(process_data);

    copy_out->copy(process_data.outputs[0].channelBuffers32, pout, numsamples);

    ((Vst::ParameterChanges*) process_data.inputParameterChanges)->clearQueue();
    ((Vst::EventList*) process_data.inputEvents)->clear();
    

    return 1;
}
