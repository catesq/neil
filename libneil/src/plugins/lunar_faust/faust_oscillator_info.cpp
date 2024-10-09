#include "faust_oscillator_info.hpp"


faust_oscillator_info::faust_oscillator_info() 
{
    this->flags = zzub::plugin_flag_has_audio_output | zzub::plugin_flag_is_instrument;
    this->min_tracks = 1;
    this->max_tracks = 1;
    this->name = "faust_oscillator";
    this->short_name = "faust_oscillator";
    this->author = "tnh";
    this->uri = "@libneil/effect/faust_oscillator";

    wave_type = &add_global_parameter()
        .set_byte()
        .set_name("wave_type")
        .set_description("wave_type")
        .set_value_min(0)
        .set_value_max(3)
        .set_value_none(255)
        .set_flags(zzub::parameter_flag_state)
        .set_value_default(0);

    freq_type = &add_global_parameter()
        .set_byte()
        .set_name("freq_type")
        .set_description("freq_type")
        .set_value_min(0)
        .set_value_max(3)
        .set_value_none(255)
        .set_flags(zzub::parameter_flag_state)
        .set_value_default(0);

    freq = &add_global_parameter()
        .set_word()
        .set_name("freq")
        .set_description("freq")
        .set_value_min(1)
        .set_value_max(20000)
        .set_value_none(0)
        .set_flags(zzub::parameter_flag_state)
        .set_value_default(1);

    // pwm = &add_global_parameter()
    //     .set_word()
    //     .set_name("pwm")
    //     .set_description("pwm")
    //     .set_value_min(0)
    //     .set_value_max(255)
    //     .set_value_none(255)
    //     .set_flags(zzub::parameter_flag_state)
    //     .set_value_default(0);

    note = &add_global_parameter()
        .set_note()
        .set_name("note")
        .set_description("note")
        .set_flags(zzub::parameter_flag_state);

    // init_phase = &add_global_parameter()
    //     .set_float()
    //     .set_name("init_phase")
    //     .set_description("init_phase")
    //     .set_value_min(0)
    //     .set_value_max(1)
    //     .set_value_none(255)
    //     .set_flags(zzub::parameter_flag_state)
    //     .set_value_default(0);

    // reset = &add_global_parameter()
    //     .set_button()
    //     .set_name("reset")
    //     .set_description("reset")
    //     .set_flags(zzub::parameter_flag_state);

    vol = &add_global_parameter()
        .set_byte()
        .set_name("vol")
        .set_description("vol")
        .set_value_min(0)
        .set_value_max(128)
        .set_value_none(255)
        .set_flags(zzub::parameter_flag_state)
        .set_value_default(80);
}