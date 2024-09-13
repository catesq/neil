#include "faust_oscillator.hpp"
#include <utility>
#include "lanternfish.h"

#define PHASE_WIDGET 0
#define RESET_WIDGET 1
#define FREQ_WIDGET 2



zzub::plugin* faust_oscillator_info::create_plugin() const {
    return new faust_oscillator(this);
}


void faust_oscillator::init(zzub::archive *arc) {
    global_values = &gval;

    std::string error_msg;

    // some faust stdlib oscillators don't have phase + reset arguments so some osc_process weren't trivial

    // two active faust instances are used to simplify ui integration
    // one instance links the ui to the phasor. 
    // the other instance is one of the four oscillatores

    // if each faust instance in osc_process had it's own sliders 
    // there would much boilerplate connecting the phase/reset/freq 
    // controls to and from zzub

    // the read position of rdtable is from phasor process
    std::string osc_process[4] = {
        // sin
        R"( 
            import("stdfaust.lib"); 

            process = rdtable(pl.tablesize, os.sinwaveform(pl.tablesize), int(_)) <:  _;
        )",

        // tri
        R"(
            import("stdfaust.lib");

            rawsaw = (os.lf_rawsaw(pl.tablesize) / pl.tablesize) - 0.5;  // -0.5 to +0.5
            tri_wave = 4 * (0.5 - abs(rawsaw)) - 1;                      // -1 to +1

            process = rdtable(pl.tablesize, tri_wave, int(_)) <: _;
        )",

        // saw
        R"(
            import("stdfaust.lib"); 

            rawsaw = 2 * (os.lf_rawsaw(pl.tablesize) / pl.tablesize) - 1;
            process = rdtable(pl.tablesize, rawsaw, int(_)) <: _;
        )",

        // sqr
        R"(
            import("stdfaust.lib"); 

            square_wave = os.sinwaveform(pl.tablesize) : <(0) : *(2) : +(-1);
            process = rdtable(pl.tablesize, square_wave, int(_)) <: _;
        )"
    };


    std::string osc_names[4] = {"sin", "tri", "saw", "sqr"};


    // uses "-I" argument in createDSPFactoryFromString to tell llvm where faustlibraries are
    const char* args[2] = { 
        "-I", 
        faust_lib_dir.c_str() 
    };

    for(int i = 0; i < 4; i++) {
        factories[i] = createDSPFactoryFromString(
            osc_names[i].c_str(),
            osc_process[i].c_str(),
            2, args,
            "",
            error_msg,
            3
        );

        if(factories[i] == nullptr)
            printf("factory %d null. error: %s\n", i, error_msg.c_str());
        else
            printf("factory %d ok\n", i);

        dsp[i] = factories[i]->createDSPInstance();
        dsp[i]->init(_master_info->samples_per_second);
    }



    // the waveforms [sine, triangle, saw, square] are stored in dsp[0-3]
    // the other faust computer for the phasor is in dsp[4]

    std::string phasor_process = R"(
        import("stdfaust.lib");

        phase = hslider("phase", 0, 0, 1, 0);
        reset = button("reset");
        freq = hslider("freq", 0, 0, 255, 1);

        phasor = os.hsp_phasor(pl.tablesize, freq, reset, phase);
        process = phasor;
    )";


    factories[4] = createDSPFactoryFromString(
        "phasor",
        phasor_process.c_str(),
        2, args,
        "",
        error_msg,
        3
    );


    if(factories[4] == nullptr)
        printf("factory %d null. error: %s\n", 4, error_msg.c_str());
    else
        printf("factory %d ok\n", 4);
    

    dsp[4] = factories[4]->createDSPInstance();
    dsp[4]->buildUserInterface(&ui);
    dsp[4]->init(_master_info->samples_per_second);


    std::vector<const char*> widget_infos = {"phase", "reset", "freq"};

    for(auto widget_name: widget_infos) {
        faust_widgets.push_back(ui.get_widget(widget_name));
    }
}


void faust_oscillator::process_events() 
{
    if (gval.wave_type != info->wave_type->value_none) {
        state.wave_type = gval.wave_type;
    }

    bool changed_freq;

    if(gval.freq_type != info->freq_type->value_none) {
        changed_freq = true;
        state.freq_type = gval.freq_type;
    }

    if(gval.freq != info->freq->value_none) {
        changed_freq = true;
        state.freq = gval.freq;
    }
    
    if(gval.note != info->note->value_none) {
        *faust_widgets[FREQ_WIDGET]->value = lanternfish::note_to_freq(gval.note);
    } else if(changed_freq) {
        *faust_widgets[FREQ_WIDGET]->value = calculate_freq(state.freq, state.freq_type);
        // _host->control_change(metaPlugin, 1, 0, port->paramIndex, zzub_val, false, true);

    }

    if(gval.vol != info->vol->value_none) {
        state.vol = gval.vol;
    }
}


float faust_oscillator::calculate_freq(unsigned short int freq, unsigned char freq_type) const
{
    switch(freq_type) {
        case 0: // Hz
            return freq;
        case 1: // milliHz
            return freq / 1000;
        case 2: // beats
            return freq / 256;
        case 3: // beats/256
            return freq / 256;
        default:
            return 0;
    }
}


bool faust_oscillator::process_mono(float **pin, float **pout, int numsamples, int mode) 
{
    if (mode == zzub::process_mode_no_io)
        return false;

    // the phasor is the faust computer to generate the phasor - from 0 to pl.tablesize
    auto phasor = dsp[4];
    auto osc = dsp[state.wave_type];

    float phase[256] = {0};
    float *phase_out[1] = {phase};

    phasor->compute(numsamples, nullptr, phase_out);

    osc->compute(numsamples, phase_out, pout);

    return true;
}


bool faust_oscillator::process_stereo(float **pin, float **pout, int numsamples, int mode) 
{
    if(!process_mono(pin, pout, numsamples, mode))
        return false;

    memcpy(pout[1], pout[0], sizeof(float) * numsamples);

    return true;
}


