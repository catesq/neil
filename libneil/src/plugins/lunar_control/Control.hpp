#ifndef LUNAR_CONTROL_HPP
#define LUNAR_CONTROL_HPP

#include <stdint.h>
#include <cmath>
#include <cstring>
#include <string>
#include <sstream>

#include <zzub/signature.h>
#include <zzub/plugin.h>

struct Gvals {
  uint16_t value;
  uint16_t power;
  uint16_t minimum;
  uint16_t maximum;
} __attribute__((__packed__));

const zzub::parameter *para_value = 0;
const zzub::parameter *para_power = 0;
const zzub::parameter *para_minimum = 0;
const zzub::parameter *para_maximum = 0;

const zzub::parameter *para_output = 0;

const char *zzub_get_signature() { 
  return ZZUB_SIGNATURE; 
}

class LunarControl : public zzub::plugin {
private:
  Gvals gval;
  uint16_t cval;
  uint16_t value;
  uint16_t power;
  uint16_t min;
  uint16_t max;
public:
  LunarControl();
  virtual ~LunarControl() {}
  virtual void init(zzub::archive* pi);
  virtual void process_events();
  virtual bool process_stereo(float **pin, float **pout, 
			      int numsamples, int mode);
  virtual bool process_offline(float **pin, float **pout, 
			       int *numsamples, int *channels, 
			       int *samplerate) { return false; }
  virtual const char * describe_value(int param, int value); 
  virtual void process_controller_events();
  virtual void destroy() {}
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
  virtual bool play_wave(int, int, float) { return false; }
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

struct LunarControlInfo : zzub::info {
  LunarControlInfo() {
    this->flags =  zzub::plugin_flag_has_event_output | zzub::plugin_flag_control_plugin;
    this->name = "Lunar Control";
    this->short_name = "Control";
    this->author = "SoMono";
    this->min_tracks = 1;
    this->max_tracks = 8;
    this->uri = "@neil/lunar/controller/Control;1";
    para_output = &add_controller_parameter()
      .set_word()
      .set_name("Out")
      .set_description("Control output")
      .set_value_min(0x0000)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_value_default(0x0000)
      .set_state_flag();
    para_value = &add_global_parameter()
      .set_word()
      .set_name("Value")
      .set_description("Control output value")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(0xffff)
      .set_value_default(0)
      .set_state_flag();
    para_power = &add_global_parameter()
      .set_word()
      .set_name("Power")
      .set_description("Amount of non-linearity")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(0xffff)
      .set_value_default(500)
      .set_state_flag();
    para_minimum = &add_global_parameter()
      .set_word()
      .set_name("Min")
      .set_description("Minimal value for output")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(0xffff)
      .set_value_default(0)
      .set_state_flag();
    para_maximum = &add_global_parameter()
      .set_word()
      .set_name("Max")
      .set_description("Maximum value for output")
      .set_value_min(0)
      .set_value_max(1000)
      .set_value_none(0xffff)
      .set_value_default(1000)
      .set_state_flag();
  }
  virtual zzub::plugin* create_plugin() const { return new LunarControl(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} MachineInfo;

struct LunarControl_PluginCollection : zzub::plugincollection {
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
  return new LunarControl_PluginCollection();
}

#endif // LUNAR_CONTROL_HPP
