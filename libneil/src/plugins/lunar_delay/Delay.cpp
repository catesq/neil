#include <algorithm>
#include <cstdio>
#include <iostream>

#include "Delay.hpp"

Svf::Svf() {
  fs = 44100;
  fc = 523;
  res = 0;
  drive = 0;
  reset();
}
	
void Svf::reset() {
  memset(v, 0, sizeof(float) * 5);
}
	
void Svf::setup(float fs, float fc, float res, float drive) {
  fs = fs;
  fc = fc;
  res = res;
  drive = drive;
  freq = 2.0 * sin(M_PI * std::min(0.25f, fc / (fs * 2)));
  damp = std::min(float(2.0f * (1.0f - pow(res, 0.25f))), 
		  std::min(2.0f, 2.0f / freq - freq * 0.5f));
}

float Svf::sample(float sample, int mode) {
  float in, out;
  // To make it possible to select only low, high and band (hackish).
  mode = mode + 1;
  in = sample;
  v[0] = (in - damp * v[3]);
  v[1] = (v[1] + freq * v[3]);
  v[2] = (v[0] - v[1]);
  v[3] = (freq * v[2] + v[3] - drive * v[3] * v[3] * v[3]);
  out = 0.5 * v[mode];
  v[0] = (in - damp * v[3]);
  v[1] = (v[1] + freq * v[3]);
  v[2] = (v[0] - v[1]);
  v[3] = (freq * v[2] + v[3] - drive * v[3] * v[3] * v[3]);
  out += 0.5 * v[mode];
  return out;    
}

float LunarDelay::peak(float *buffer, int n) {
  float abs_peak;
  float peak_value = fabs(buffer[0]);
  for (int i = 1; i < n; i++) {
    abs_peak = fabs(buffer[i]);
    if (abs_peak > peak_value) {
      peak_value = abs_peak;
    }
  }
  return peak_value;
}

float LunarDelay::squash(float x) {
  if (x >= 1.0) {
    return 1.0;
  } else if (x <= -1.0) {
    return -1.0;
  } else {
    return 1.5 * x - 0.5 * x * x * x;
  }
}

bool LunarDelay::rb_empty(ringbuffer_t *rb) {
  /* 
     Checks if a ringbuffer rb is empty.
     Iterate through the buffer until the end is reached
     if any one sample exceeds the threshold return false,
     otherwise return true.
  */
  float *pointer = rb->buffer;
  while (pointer++ != rb->eob) {
    if (fabs(*pointer) > 0.0001f) {
      return false;
    }
  }
  return true;
}

void LunarDelay::rb_init(ringbuffer_t *rb) {
  for (int i = 0; i < MAX_DELAY_LENGTH; i++) {
    rb->buffer[i] = 0.0f;
  }
  rb->eob = rb->buffer + 1;
  rb->pos = rb->buffer;
  rb->rpos = rb->pos;
}

void LunarDelay::rb_setup(ringbuffer_t *rb, int size) {
  rb->eob = rb->buffer + size;
  while (rb->pos >= rb->eob)
    rb->pos -= size;
}

void LunarDelay::rb_mix(ringbuffer_t *rb, Svf *filter, float **out, int n) {
  float sl, sr;
  while (n--) {
    sl = *out[0];
    sr = *out[1];
    if (mode == 0 || mode == 1) {
      *out[0] = (*out[0] * dry) + (*rb[0].pos * wet);
      *out[1] = (*out[1] * dry) + (*rb[1].pos * wet);
    } else {
      *out[0] = (*out[0] * dry) + (*rb[0].rpos * wet);
      *out[1] = (*out[1] * dry) + (*rb[1].rpos * wet);
    }
    if (mode == 0 || mode == 2) {
      *rb[0].pos = std::min(std::max((*rb[0].pos * fb) + sl, -1.0f), 1.0f);
      *rb[1].pos = std::min(std::max((*rb[1].pos * fb) + sr, -1.0f), 1.0f);
    } else {
      *rb[0].pos = std::min(std::max((*rb[1].pos * fb) + sl, -1.0f), 1.0f);
      *rb[1].pos = std::min(std::max((*rb[0].pos * fb) + sr, -1.0f), 1.0f);
    }
    *rb[0].pos = filter[0].sample(*rb[0].pos, filter_mode);
    *rb[0].pos = squash(*rb[0].pos);
    *rb[1].pos = filter[1].sample(*rb[1].pos, filter_mode);
    *rb[1].pos = squash(*rb[1].pos);
    out[0]++;
    out[1]++;
    rb[0].pos++;
    rb[1].pos++;
    if (rb[0].pos == rb[0].eob) {
      rb[0].pos = rb[0].buffer;
    }
    if (rb[1].pos == rb[1].eob) {
      rb[1].pos = rb[1].buffer;
    }
    if (rb[0].rpos == rb[0].buffer) {
      rb[0].rpos = rb[0].eob;
    }
    if (rb[1].rpos == rb[1].buffer) {
      rb[1].rpos = rb[1].eob;
    }
    rb[0].rpos--;
    rb[1].rpos--;
  }
}

LunarDelay::LunarDelay() {
  global_values = &gval;
  attributes = 0;
  track_values = 0;
  last_empty = true;
}

void LunarDelay::init(zzub::archive *pi) {
  rb_init(&rb[0]);
  rb_init(&rb[1]);
  wet     = 0.0f;
  dry     = 0.0f;
  fb      = 0.0f;
  l_incr  = -1;
  r_incr  = -1;
  l_count = 0;
  r_count = 0;

#ifdef USE_CUTOFF_NOTE
  cutoff_note       = para_cutoff_note->value_default;
  cutoff_cents      = para_cutoff_cents->value_default;
  note_to_freq_base = pow(2.0, 1.0/1200.0);
  meta_plugin       = _host->get_metaplugin();
#endif
}


void LunarDelay::update_buffer() {
  ldelay = ldelay_ticks * _master_info->samples_per_tick;
  rdelay = rdelay_ticks * _master_info->samples_per_tick;
  int rbsizel = std::min((int)ldelay, MAX_DELAY_LENGTH);
  int rbsizer = std::min((int)rdelay, MAX_DELAY_LENGTH);
  rb_setup(&rb[0], rbsizel);
  rb_setup(&rb[1], rbsizer);
}
	
void LunarDelay::process_events() {
  int update = 0;
  bool update_cutoff_from_note = false;
  if (gval.l_delay_ticks != 0xffff) {
    ldelay_ticks = 0.25 + 
      (gval.l_delay_ticks / (float)para_l_delay_ticks->value_max) * 
      (32.0 - 0.25);
  }
  if (gval.r_delay_ticks != 0xffff) {
    rdelay_ticks = 0.25 +
      (gval.r_delay_ticks / (float)para_r_delay_ticks->value_max) *
      (32.0 - 0.25);
  }

  if (gval.filter_mode != 0xff) {
    filter_mode = gval.filter_mode;
    update = 1;
  }


#ifdef USE_CUTOFF_NOTE
  if (gval.cutoff_note != para_cutoff_note->value_none) {
    cutoff_note = gval.cutoff_note;
    update_cutoff_from_note = true;
  }

  if (gval.cutoff_cents != para_cutoff_cents->value_none) {
      cutoff_cents = gval.cutoff_cents - 128;
      update_cutoff_from_note = true;
  }

  if(update_cutoff_from_note) {
    cutoff = (float) note_params_to_freq(cutoff_note, cutoff_cents);

    // the _host is null when process_events() is first called
    if(!first_event_process) {
        _host->set_parameter(meta_plugin, 1, 0, PARAM_CUTOFF_FREQ, (int) cutoff);
        zzub_event_data event_data = { zzub_event_type_parameter_changed };
        event_data.change_parameter.plugin = meta_plugin;
        event_data.change_parameter.group = 1;
        event_data.change_parameter.track = 0;
        event_data.change_parameter.param = PARAM_CUTOFF_FREQ;
        event_data.change_parameter.value = (int) cutoff;
        zzub_plugin_invoke_event(meta_plugin, &event_data, false);
    }

    gval.cutoff = (int) cutoff;
    update = 1;
    first_event_process = false;
  }
#endif

  if (gval.cutoff != 0xffff) {
    cutoff = (float)gval.cutoff;
    update = 1;
  }
  if (gval.resonance != 0xffff) {
    resonance = gval.resonance / (float)para_resonance->value_max;
    update = 1;
  }
  if (gval.wet != 0xffff) {
    float val = -48.0 + (gval.wet / (float)para_wet->value_max) * 60.0;
    wet = dbtoamp(val, -48.0f);
  }
  if (gval.dry != 0xffff) {
    float val = -48.0 + (gval.dry / (float)para_dry->value_max) * 60.0;
    dry = dbtoamp(val, -48.0f);
  }
  if (gval.fb != 0xffff) {
    float val = -48.0 + (gval.fb / (float)para_fb->value_max) * 60.0;
    fb = dbtoamp(val, -48.0f);
  }
  if (gval.mode != 0xff) {
    mode = gval.mode;
  }
  if (update == 1) {
    int srate = _master_info->samples_per_second;
    for (int i = 0; i < 2; i++) {
      filters[i].setup((float)srate, this->cutoff, this->resonance, 0.0);
    }
  }

}

bool LunarDelay::process_stereo(float **pin, float **pout, int n, int mode) {
  update_buffer();
  for (int i = 0; i < n; i++) {
    pout[0][i] = pin[0][i];
    pout[1][i] = pin[1][i];
  }
  rb_mix(rb, filters, pout, n);
  return true;
}

#ifdef USE_CUTOFF_NOTE

unsigned
LunarDelay::note_params_to_freq(int note_index, int note_cents) {
    float num_semitones = 100.0f * (note_index - DISPLAY_NOTE_A3) + (float)(note_cents);
    float note_freq     = 440.0f * pow(note_to_freq_base, num_semitones);
    return std::min(para_cutoff->value_max, std::max(para_cutoff->value_min, (int) note_freq));
}

#endif


const char *LunarDelay::describe_value(int param, int value) {

  static char txt[24];
  switch(param) {
  case PARAM_LDELAY_TICKS:
    sprintf(txt, "%.2f ticks", ldelay_ticks);
    break;
  case PARAM_RDELAY_TICKS:
    sprintf(txt, "%.2f ticks", rdelay_ticks);
    break;
  case PARAM_FILTER_MODE:
    switch(filter_mode) {
    case 0:
      sprintf(txt, "Low");
      break;
    case 1:
      sprintf(txt, "High");
      break;
    case 2:
      sprintf(txt, "Band");
      break;
    default:
      sprintf(txt, "???");
      break;
    }
    break;
#ifdef USE_CUTOFF_NOTE
  case PARAM_CUTOFF_NOTE:
    sprintf(txt, "%s (%5u Hz)", note_param_to_str(value).c_str(), note_params_to_freq(value, cutoff_cents));
    break;
  case PARAM_CUTOFF_CENTS:
    sprintf(txt, "%4d (%5u Hz)", value - 128, note_params_to_freq(cutoff_note, value - 128));
    break;
#endif
  case PARAM_CUTOFF_FREQ:
    sprintf(txt, "%d Hz", (int)cutoff);
    break;
  case PARAM_RESONANCE:
    sprintf(txt, "%.2f", resonance);
    break;
  case PARAM_FEEDBACK:
    sprintf(txt, "%.2f", 10.0 * log10(fb));
    break;
  case PARAM_WET_AMP:
    sprintf(txt, "%.2f", 10.0 * log10(wet));
    break;
  case PARAM_DRY_AMP:
    sprintf(txt, "%.2f", 10.0 * log10(dry));
    break;
  case PARAM_DIRECTION:
    switch(value) {
    case 0: 
      sprintf(txt, "Straight");
      break;
    case 1:
      sprintf(txt, "Ping-Pong");
      break;
    case 2:
      sprintf(txt, "Reverse");
      break;
    case 3:
      sprintf(txt, "Rev. Ping-Pong");
      break;
    }
    break;
  default:
    return 0;
  }
  return txt;
}
