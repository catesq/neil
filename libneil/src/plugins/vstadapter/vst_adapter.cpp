#include "vst_adapter.h"

#include <boost/predef/other/endian.h>
#include <gtk/gtk.h>

#include <bit>
#include <boost/endian/conversion.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "vst_defines.h"
#include "vst_plugin_info.h"
#include "vstfxstore.h"

#include "loguru.hpp"

#include "libzzub/timer.h"

//

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

#define VSTADAPTER(effect) ((VstAdapter*)effect->resvd1)
// #endif


extern "C" {
    void on_window_destroy(GtkWidget* widget, gpointer data) {
        static_cast<vst_adapter*>(data)->ui_destroy();
    }
}


/// TODO proper set/get parameter handling and opcodes: 42,43 & 44 used by oxefm
VstIntPtr
VSTCALLBACK hostCallback(
    AEffect* effect,
    VstInt32 opcode,
    VstInt32 index,
    VstIntPtr value,
    void* ptr,
    float opt
) {

    // the first use to hostCollback is during vst startup so effect->resvd1 is undefined
    if (opcode == audioMasterVersion) {
        return 2400;
    } else if (!effect || !effect->resvd1) {
        return 0;
    }

    vst_adapter* adapter = (vst_adapter*)effect->resvd1;

    switch (opcode) {
        case audioMasterBeginEdit:
            LOG_F(INFO, "hostCallback: opcode begin edit");
            
            // printf("opcode begin edit\n");
            break;

        case audioMasterEndEdit:  // so far I can get the same info from audioMasterAutomate
            LOG_F(INFO, "hostCallback: opcode end edit");
            // printf("opcode end edit\n");
            break;

        case audioMasterAutomate:
        case audioMasterUpdateDisplay:
            printf("\nupdating from ui: index %d, val %.2f\n", index, opt);
            adapter->parameter_update_from_ui(index, opt);
            break;

        case audioMasterProcessEvents: {
            VstEvents* events = (VstEvents*)ptr;
            printf("vst events COUNT: %d\n", events->numEvents);
            break;
        }

        case audioMasterGetSampleRate:
            return adapter->_master_info->samples_per_second;

        case audioMasterGetBlockSize:
            return zzub_buffer_size;

        case audioMasterIdle:
            dispatch(effect, effEditIdle);
            break;

        case audioMasterGetTime:
            return (VstIntPtr)adapter->get_vst_time_info();

        case audioMasterGetProductString:
            strncpy((char*)ptr, "Neil modular sequencer", kVstMaxProductStrLen);
            return true;

        case audioMasterGetCurrentProcessLevel:
            return kVstProcessLevelUnknown;

        case audioMasterSizeWindow:
            adapter->ui_resize(value, index);
            break;

        case DECLARE_VST_DEPRECATED(audioMasterWantMidi):
            return 0;

        default:
            LOG_F(WARNING, "vst hostCallback: missing opcode %d index %d\n", opcode, index);
            break;
    }

    return 0;
}


vst_adapter::vst_adapter(const vst_zzub_info* info) 
  : info(info), midi_track_manager(*this) {

    globalvals = (uint16_t*)malloc(sizeof(uint16_t) * info->get_param_count());
    attributes = (int*)&attr_values;
    global_values = globalvals;

    if (info->flags & zzub_plugin_flag_has_midi_input) {
        track_values = midi_track_manager.get_track_data();
        num_tracks = 1;
        set_track_count(num_tracks);
    }

    memset(&vst_time_info, 0, sizeof(VstTimeInfo));
    vst_events = (VstEvents*)malloc(sizeof(VstEvents) + sizeof(VstEvent*) * (MAX_EVENTS - 2));
    vst_events->numEvents = 0;
    vst_time_info.flags = kVstTempoValid | kVstTransportPlaying;
}


vst_adapter::~vst_adapter() {
    dispatch(plugin, effMainsChanged, 0, 0, NULL, 0.0f);
    dispatch(plugin, effClose);

    if (is_editor_open)
        ui_destroy();

    clear_vst_events();
    free(globalvals);
    free(vst_events);

    free(copy_in);
    free(copy_out);
}


void 
vst_adapter::clear_vst_events() {
    for (auto evt : midi_events)
        free(evt);

    // for(int idx=0; idx < vst_events->numEvents; idx++) {
    //     free(vst_events->events[idx]);
    //     vst_events->events[idx] = nullptr;
    // }

    midi_events.clear();

    memset(vst_events->events, 0, sizeof(VstEvent*) * vst_events->numEvents);
    vst_events->numEvents = 0;
}


void 
vst_adapter::set_track_count(int track_count) {
    midi_track_manager.set_track_count(track_count);
    num_tracks = track_count;
}


VstTimeInfo*
vst_adapter::get_vst_time_info() {
    vst_time_info.samplePos = sample_pos;
    vst_time_info.sampleRate = _master_info->samples_per_second;
    vst_time_info.tempo = _master_info->beats_per_minute;

    return &vst_time_info;
}


void 
vst_adapter::init(zzub::archive* arc) {
    plugin = load_vst(lib, info->get_filename(), hostCallback, this);

    if (plugin == nullptr)
        return;

    metaplugin = _host->get_metaplugin();
    _host->set_event_handler(metaplugin, this);
    _host->add_plugin_event_listener(zzub::event_type_edit_pattern, this);

    ui_scale = gtk_widget_get_scale_factor((GtkWidget*)_host->get_host_info()->host_ptr);

    if (info->flags & zzub_plugin_flag_has_midi_input) {
        midi_track_manager.init(_master_info->samples_per_second);
    }

    dispatch(plugin, effOpen);
    dispatch(plugin, effSetSampleRate, 0, 0, nullptr, (float) _master_info->samples_per_second);
    dispatch(plugin, effSetBlockSize, 0, (VstIntPtr) zzub_buffer_size, nullptr, 0);

    audioIn = init_audio_buffers(plugin->numInputs);
    audioOut = init_audio_buffers(plugin->numOutputs);
    copy_in = zzub::tools::CopyChannels::build(2, plugin->numInputs);
    copy_out = zzub::tools::CopyChannels::build(plugin->numOutputs, 2);

    dispatch(plugin, effMainsChanged, 0, 1, NULL, 0.0f);

    if (arc)  // use  plugin state
        init_from_archive(arc);
}

float** 
vst_adapter::init_audio_buffers(int count) {
    if (count == 0)
        return nullptr;

    float** buffers = (float**) malloc(sizeof(float*) * count);

    for(int idx = 0; idx < count; idx++) {
        buffers[idx] = (float*) malloc(sizeof(float) * zzub_buffer_size);
    }

    return buffers;
}

// reset vst parameters when loading from a save file
void 
vst_adapter::init_from_archive(zzub::archive* arc) {
    zzub::instream* pi = arc->get_instream("");
    uint16_t param_count = 0;
    pi->read(&param_count, sizeof(uint16_t));

    if (param_count != info->global_parameters.size())
        return;

    float vst_val;
    for (uint16_t idx = 0; idx < param_count; idx++) {
        pi->read(&vst_val, sizeof(float));
        plugin->setParameter(plugin, idx, vst_val);
    }

    return;
}


// completes the recreation of the plugin when loading a save file
// when inserting a new vst it will set the parameters on the zzub side to
void 
vst_adapter::created() {
    initialized = true;

    for (vst_parameter* param : info->get_vst_params()) {
        float vst_val = plugin->getParameter(plugin, param->index);
        _host->set_parameter(metaplugin, 1, 0, param->index, param->vst_to_zzub_value(vst_val));
    }
}


// simple save of plugin state. just store the current value of all the parameters
// should work for most plugins. more complex ones might need to use the save_preset
void 
vst_adapter::save(zzub::archive* arc) {
    zzub::outstream* po = arc->get_outstream("");

    uint16_t param_count = info->global_parameters.size();
    po->write(&param_count, sizeof(uint16_t));

    for (uint16_t idx = 0; idx < param_count; idx++) {
        float vst_val = plugin->getParameter(plugin, idx);
        po->write(&vst_val, sizeof(float));
    }
}

void 
vst_adapter::ui_resize(int width, int height) {
    if (window)
        gtk_widget_set_size_request(window, width / ui_scale, height / ui_scale);
}


bool 
vst_adapter::invoke(zzub_event_data_t& data) {
    if(!plugin)
        return false;

    switch (data.type) {
        case zzub::event_type_edit_pattern:
        printf("parameter update group %d\n", data.edit_pattern.group);
            if (data.edit_pattern.group == zzub_parameter_group_track) {
                midi_track_manager.parameter_update(data.edit_pattern.track, data.edit_pattern.column, data.edit_pattern.row, data.edit_pattern.value);
            }
            break;

        case zzub::event_type_double_click:
            if (!is_editor_open && (info->flags & zzub_plugin_flag_has_custom_gui)) {
                ui_open();
            }
            break;
    }

    return true;
}


void 
vst_adapter::ui_open() {
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), this);

    gtk_window_set_title(GTK_WINDOW(window), info->name.c_str());
    gtk_window_present(GTK_WINDOW(window));

    auto win_id = WIN_ID_FUNC(window);

    dispatch(plugin, effEditOpen, 0, 0, (void*)win_id, 0);
    gtk_widget_show_all(window);
    
    ERect* gui_size = nullptr;
    dispatch(plugin, effEditGetRect, 0, 0, (void*)&gui_size, 0);
    dispatch(plugin, effGetProgram, 0, 0, nullptr, 0);

    if (gui_size) {
        LOG_F(INFO, "vst_adapter: open gui %d %d %d %d", gui_size->left, gui_size->right, gui_size->top, gui_size->bottom);
        ui_resize(gui_size->right, gui_size->bottom);
    }

    dispatch(plugin, __effIdleDeprecated);
    dispatch(plugin, effEditIdle);

    is_editor_open = true;
    idle_task_id = g_timeout_add(100, [](void* data) -> gboolean { dispatch((AEffect*) data, __effIdleDeprecated); return true; }, plugin);
}


void 
vst_adapter::ui_destroy() {
    dispatch(plugin, effEditClose);
    is_editor_open = false;
    g_source_remove (idle_task_id);
}


void 
vst_adapter::parameter_update_from_ui(int idx, float float_val) {
    printf("update param from gui: index = %d, name: %s,  vst val = %f, zzub val = %d \n", idx, info->get_vst_param(idx)->zzub_param->name, float_val, info->get_vst_param(idx)->vst_to_zzub_value(float_val));
    globalvals[idx] = info->get_vst_param(idx)->vst_to_zzub_value(float_val);
    _host->control_change(metaplugin, 1, 0, idx, globalvals[idx], false, true);
    //    zzub_plugin_set_parameter_value_direct(metaplugin, 1, 0, idx, globalvals[idx], false);
}


void 
vst_adapter::process_events() {
    if (!initialized)
        return;

    for (auto idx = 0; idx < info->get_param_count(); idx++) {
        auto vst_param = info->get_vst_param(idx);
        uint16_t value = globalvals[idx];

        if (value == vst_param->zzub_param->value_none)
            continue;

        ((AEffectSetParameterProc)plugin->setParameter)(plugin, idx, vst_param->zzub_to_vst_value(value));
    }

    if (info->flags & zzub_plugin_flag_has_midi_input)
        midi_track_manager.process_events();
}



void 
vst_adapter::add_note_on(uint8_t note, uint8_t volume) {
    LOG_BEAT("vst_adapter add_note_on", _master_info, sample_pos);

    if (midi_events.size() < MAX_EVENTS)
        midi_events.push_back(midi_note_on(note, volume));
}

void 
vst_adapter::add_note_off(uint8_t note) {
    LOG_BEAT("vst_adapter add_note_off", _master_info, sample_pos);

    if (midi_events.size() < MAX_EVENTS)
        midi_events.insert(midi_events.begin(), midi_note_off(note));
}


void 
vst_adapter::add_aftertouch(uint8_t note, uint8_t volume) {
    LOG_BEAT("vst_adapter add_aftertouch", _master_info, sample_pos);

    if (midi_events.size() < MAX_EVENTS)
        midi_events.push_back(midi_note_aftertouch(note, volume));
}


void 
vst_adapter::add_midi_command(uint8_t cmd, uint8_t data1, uint8_t data2) {
    LOG_BEAT("vst_adapter add_midi_command", _master_info, sample_pos);

    if (midi_events.size() < MAX_EVENTS)
        midi_events.push_back(midi_message(cmd, data1, data2));
}


bool 
vst_adapter::process_stereo(float** pin, float** pout, int numsamples, int mode) {
    sample_pos += numsamples;

    if(sample_pos - sample_count_last_idle > 5000) {
        dispatch(plugin, __effIdleDeprecated);
        dispatch(plugin, effEditIdle);
        sample_count_last_idle = sample_pos;
    }

    if (info->flags & zzub_plugin_flag_has_midi_input) {
        midi_track_manager.process_samples(numsamples, mode);

        if (midi_events.size() > 0) {
            printf("vst_adapter has midi events\n");
            vst_events->numEvents = midi_events.size();
            memcpy(&vst_events->events[0], &midi_events[0], vst_events->numEvents * sizeof(VstMidiEvent*));
            dispatch(plugin, effProcessEvents, 0, 0, vst_events, 0.f);
            clear_vst_events();
        }
    }

    if (mode == zzub::process_mode_no_io)
        return 1;

    copy_in->copy(pin, audioIn, numsamples);

    plugin->processReplacing(plugin, audioIn, audioOut, numsamples);

    copy_out->copy(audioOut, pout, numsamples);

    return 1;
}


const char*
vst_adapter::get_preset_file_extensions() {
    return "Preset file=.fxp:Preset bank=.fxb";
}


bool 
vst_adapter::save_preset_file(const char* filename) {
    printf("Save preset: %s\n", filename);
    std::filesystem::path path{filename};
    struct fxProgram program {};

    void* chunk_data;
    // ask for the chunk data for a vst Program
    int chunk_size = dispatch(plugin, effGetChunk, 1, 0, &chunk_data, 0);

    // see if effGetChunk worked.
    bool save_chunk = true;
    if (!chunk_data) {
        save_chunk = false;
    } else if (chunk_size == 0) {
        save_chunk = false;

        if (chunk_data)
            free(chunk_data);

        chunk_data = nullptr;
    }

    // at this poinFt we know for sure if chunk_data is valid
    // if so then save it, if not then write each param to save file
    if (save_chunk) {
        // TODO very important save preset stuff

    } else {
        chunk_data = malloc(info->get_param_count() * sizeof(float));

        if (!chunk_data)
            goto save_preset_end_false;

        float* curr_param = (float*)chunk_data;
        for (int idx = 0; idx < info->get_param_count(); idx++) {
            *curr_param++ = info->get_vst_param(idx)->zzub_to_vst_value(globalvals[idx]);
        }
    }

save_preset_end_true:
    if (chunk_data)
        free(chunk_data);
    return true;

save_preset_end_false:
    return false;
}


bool 
vst_adapter::load_preset_file(const char* filename) {
    printf("Load preset: %s\n", filename);

    std::filesystem::path path{filename};

    if (!std::filesystem::is_regular_file(path))
        return false;

    auto fsize = std::filesystem::file_size(path);

    if (fsize < sizeof(fxProgram))
        return false;

    std::ifstream file(filename);

    if (!file.is_open())
        return false;

    char* data = (char*)malloc(fsize);

    if (!data)
        return false;

    file.read(data, fsize);

    file.close();

    struct fxProgram* program = (struct fxProgram*)data;
    struct fxBank* bank = (struct fxBank*)data;

#ifdef BOOST_ENDIAN_LITTLE_BYTE
    boost::endian::big_to_native_inplace(program->chunkMagic);
    boost::endian::big_to_native_inplace(program->fxMagic);
    boost::endian::big_to_native_inplace(program->byteSize);
    boost::endian::big_to_native_inplace(program->version);
    boost::endian::big_to_native_inplace(program->fxID);
    boost::endian::big_to_native_inplace(program->fxVersion);
    boost::endian::big_to_native_inplace(program->numParams);
#endif

    if (program->chunkMagic != cMagic)
        goto load_preset_end_false;

    if (program->fxID != plugin->uniqueID)
        goto load_preset_end_false;

    switch (program->fxMagic) {
        case fMagic:
            for (int index = 0; index < program->numParams; index++) {
                boost::endian::big_to_native_inplace(program->content.params[index]);
                plugin->setParameter(plugin, index, program->content.params[index]);
            }
            break;

        case chunkPresetMagic:
            boost::endian::big_to_native_inplace(program->content.data.size);
            dispatch(plugin, effSetChunk, 1, program->content.data.size, program->content.data.chunk, 0);
            break;

        case bankMagic:
            break;

        case chunkBankMagic:
            break;
    }

    // load_preset_end_true:
    //  update_zzub_params = true;
    free(data);
    return true;

load_preset_end_false:
    free(data);
    return false;
}


const char*
vst_adapter::describe_value(int param, int value) {
    static char description_chars[32];

    if (param < 0) {
        return nullptr;
    }

    std::string description_str;
    
    if (param < info->get_param_count()) {
        description_str = std::to_string(value);
    } else {
        int track_param = param - info->get_param_count();
        description_str = midi_track_manager.describe_value(track_param / 6, track_param % 6, value);
    }

    std::strncpy(description_chars, description_str.c_str(), 32);

    return description_chars;
}
