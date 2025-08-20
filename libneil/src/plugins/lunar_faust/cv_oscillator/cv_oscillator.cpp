#include "cv_oscillator.hpp"
#include <utility>
#include "lanternfish.h"

#define PHASE_WIDGET 0
#define RESET_WIDGET 1
#define FREQ_WIDGET 2


zzub::plugin* cv_oscillator_info::create_plugin() const {
    return new cv_oscillator(this);
}


void cv_oscillator::init(zzub::archive *arc) {
    global_values = &gval;
    metaplugin = _host->get_metaplugin();
    
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

        assert(factories[i] != nullptr);

        dsp[i] = factories[i]->createDSPInstance();
        dsp[i]->init(_master_info->samples_per_second);
    }



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

    assert(factories[4] != nullptr);
    

    dsp[4] = factories[4]->createDSPInstance();
    dsp[4]->buildUserInterface(&ui);
    dsp[4]->init(_master_info->samples_per_second);


    std::vector<const char*> widget_infos = {"phase", "reset", "freq"};

    for(auto widget_name: widget_infos) {
        faust_widgets.push_back(ui.get_widget(widget_name));
    }

    init_port_facade(_host, {&lfo});
}


void cv_oscillator::process_events() 
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
    }

    if(gval.vol != info->vol->value_none) {
        state.vol = gval.vol;
    }
}


float cv_oscillator::calculate_freq(unsigned short int freq, unsigned char freq_type) const
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


void cv_oscillator::process_cv(int numsamples) 
{
    // the phasor generates a offset into a faust rdtable from 0 to tablesize
    auto phasor = dsp[4];

    // the oscillator generates the wavetable which the phasor indexes
    auto osc = dsp[state.wave_type];

    phasor->compute(numsamples, nullptr, &phase);
    osc->compute(numsamples, &phase, &output);
}


