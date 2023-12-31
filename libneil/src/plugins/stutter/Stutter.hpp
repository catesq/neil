#ifndef SOMONO_STUTTER_HPP
#define SOMONO_STUTTER_HPP

#include <stdint.h>

#include <zzub/signature.h>
#include <zzub/plugin.h>

struct Gvals {
  uint8_t length;
  uint8_t time;
  uint8_t smoothing;
} __attribute__((__packed__));

const zzub::parameter *param_length = 0;
const zzub::parameter *param_time = 0;
const zzub::parameter *param_smoothing = 0;

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class Stutter : public zzub::plugin {
private:
  Gvals gval;
  float *buffer[2];
  bool record;
  int cursor, counter, length, time;
  float tick_length, smoothing;
  float envelope(int cursor, int length);
public:
  Stutter();
  virtual ~Stutter();
  virtual void init(zzub::archive* pi);
  virtual void process_events();
  virtual bool process_stereo(float **pin, float **pout, 
			      int numsamples, int mode);
  virtual bool process_offline(float **pin, float **pout, 
			       int *numsamples, int *channels, 
			       int *samplerate) { return false; }
  virtual const char * describe_value(int param, int value); 
  virtual void process_controller_events() {}
  virtual void destroy();
  virtual void stop() {}
  virtual void load(zzub::archive *arc) {}
  virtual void save(zzub::archive*) {}
  virtual void attributes_changed() {}
  virtual void command(int) {}
  virtual void set_track_count(int) {}
  virtual void mute_track(int) {}
  virtual bool is_track_muted(int) const { return false; }
  virtual void midi_note(int, int, int) {}
  virtual void event(unsigned int) {}
  virtual const zzub::envelope_info** get_envelope_infos() { return 0; }
  virtual void stop_wave() {}
  virtual int get_wave_envelope_play_position(int) { return -1; }
  virtual const char* describe_param(int) { return 0; }
  virtual bool set_instrument(const char*) { return false; }
  virtual void get_sub_menu(int, zzub::outstream*) {}
  virtual void add_input(const char*, zzub::connection_type) {}
  virtual void delete_input(const char*, zzub::connection_type) {}
  virtual void rename_input(const char*, const char*) {}
  virtual void input(float**, int, float) {}
  virtual void midi_control_change(int, int, int) {}
  virtual bool handle_input(int, int, int) { return false; }
  virtual void process_midi_events(zzub::midi_message* pin, int nummessages) {}
  virtual void get_midi_output_names(zzub::outstream *pout) {}
  virtual void set_stream_source(const char* resource) {}
  virtual const char* get_stream_source() { return 0; }
  virtual void play_pattern(int index) {}
  virtual void configure(const char *key, const char *value) {}
};

struct StutterInfo : zzub::info {
  StutterInfo() {
    this->flags = 
      zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output | zzub::plugin_flag_is_effect;
    this->name = "SoMono Stutter";
    this->short_name = "Stutter";
    this->author = "SoMono";
    this->uri = "@libneil/somono/effect/stutter;1";
    param_length = &add_global_parameter()
      .set_byte()
      .set_name("Length")
      .set_description("Sampling window size in subticks")
      .set_value_min(0x01)
      .set_value_max(0xff)
      .set_value_none(0x00)
      .set_state_flag()
      .set_value_default(0x10);
    param_time = &add_global_parameter()
      .set_byte()
      .set_name("Time")
      .set_description("Running time in ticks")
      .set_value_min(0x01)
      .set_value_max(0x20)
      .set_value_none(0xFF)
      .set_state_flag()
      .set_value_default(0x01);
    param_smoothing = &add_global_parameter()
      .set_byte()
      .set_name("Smoothing")
      .set_description("Smoothing amount")
      .set_value_min(0x00)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_state_flag()
      .set_value_default(0x80);
  }
  virtual zzub::plugin* create_plugin() const { return new Stutter(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct SoMono_Stutter_PluginCollection : zzub::plugincollection {
  virtual void initialize(zzub::pluginfactory *factory) {
    factory->register_info(&MachineInfo);
  }
  virtual const zzub::info *get_info(const char *uri, zzub::archive *data) { 
    return 0; 
  }
  virtual void destroy() { 
    delete this; 
  }
  virtual const char *get_uri() { 
    return 0;
  }
  virtual void configure(const char *key, const char *value) {

  }
};

zzub::plugincollection *zzub_get_plugincollection() {
  return new SoMono_Stutter_PluginCollection();
}

#endif // SOMONO_STUTTER_HPP
