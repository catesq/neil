#include "faust_oscillator.hpp"

#define PHASE_WIDGET = 0;
#define RESET_WIDGET = 1;
#define FREQ_WIDGET = 2;

void faust_oscillator::init(zzub::archive *arc) {
    global_values = &gval;
    std::string error_msg;
    
    std::string phasor = R"(
        tablesize = 1024;
        phase = hslider("phase", 0, 0, 1, 0.01);
        reset = button("reset");
        freq = hslider("freq", 0, 0, 255, 1);

        phasor = os.hsp_phasor(tableSize, freq, reset, phase);

        process = phasor;
    )";

    std::string faust_pre = R"(
        import("stdfaust.lib"); 

        tablesize = 1024;
        wave = rdtable(tablesize, 
    )";


    std::string  faust_post = R"( );
        
        process = int(_) : wavetable;
    )";


    std::string wavegen[4] = {
        "sin(2*PI*float(i) / float(tablesize))",
        R"(
            i < tableSize / 2 ? 
            (2 * float(i) / float(tableSize / 2)) - 1 : 
            1 - (2 * float(i - tableSize / 2) / float(tableSize / 2)) 
        )",
        "lf_rawsaw(tablesize)",
        "i : i < tableSize/2 ? 1 : -1"
    };


    for(int i = 0; i < 4; i++) {
        std::string faust_code = faust_pre + wavegen[i] + faust_post;

        factories[i] = createDSPFactoryFromString(
            "faust",
            faust_code.c_str(),
            0, nullptr,
            "",
            error_msg,
            3
        );

        printf("factory %d is null: %d\n", i, factories[i] == nullptr);

        printf("error: %s\n", error_msg.c_str());


        dsp[i] = factories[i]->createDSPInstance();
        dsp[i]->init(_master_info->samples_per_second);
    }


    factories[4] = createDSPFactoryFromString(
        "faust",
        phasor.c_str(),
        0, nullptr,
        "",
        error_msg,
        3
    );

    dsp[4] = factories[4]->createDSPInstance();
    dsp[4]->init(_master_info->samples_per_second);

    printf("num phasor inputs: %d\n", dsp[4]->getNumInputs());
    printf("num phasor outputs: %d\n", dsp[4]->getNumOutputs());
    printf("num rdtable inputs: %d\n", dsp[2]->getNumInputs());
    printf("num rdtableoutputs: %d\n", dsp[2]->getNumOutputs());

}


bool faust_oscillator::process_stereo(float **pin, float **pout, int numsamples, int mode) 
{
    if (mode == zzub::process_mode_no_io)
        return false;

    
    
    return true;
}

    // zzub::parameter *wave_type;    // sin,tri,saw,sqr
    // zzub::parameter *freq_type;    // Hz, milliHz, beats, beats/256
    // zzub::parameter *freq;         // 0 - 32767
    // zzub::parameter *pwm;
    // zzub::parameter *note;
    // zzub::parameter *vol;


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
        .set_value_min(0)
        .set_value_max(255)
        .set_value_none(255)
        .set_flags(zzub::parameter_flag_state)
        .set_value_default(0);

    pwm = &add_global_parameter()
        .set_word()
        .set_name("pwm")
        .set_description("pwm")
        .set_value_min(0)
        .set_value_max(255)
        .set_value_none(255)
        .set_flags(zzub::parameter_flag_state)
        .set_value_default(0);

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