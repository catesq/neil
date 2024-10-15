#pragma once

#include <zzub/plugin.h>
#include <zzub/signature.h>
#include <zzub/zzub.h>

#include "libzzub/ports/ports_facade.h"

#include "faust/dsp/llvm-dsp.h"
#include "faust_ui.hpp"

#include "faust_oscillator_info.hpp"


const char* zzub_get_signature()
{
    return ZZUB_SIGNATURE;
}

class cv_data_port : public zzub::port {
    std::string port_name;
    const zzub::port_flow direction;
    std::vector<float> data;

public:
    cv_data_port(
        std::string name,
        const zzub::port_flow direction,
        int size = zzub_buffer_size
    )
        : port_name(name)
        , direction(direction)
    {
        data.resize(size);
    }

    virtual ~cv_data_port() { }


    virtual const char* get_name() override
    {
        return port_name.c_str();
    }


    virtual zzub::port_flow get_flow() override
    {
        return direction;
    }


    virtual zzub::port_type get_type() override
    {
        return zzub::port_type::cv;
    }
};


class gvals {
public:
    unsigned char wave_type = 0; // sin,tri,saw,sqr
    unsigned char freq_type = 0; // Hz, milliHz, beats, beats/192, beats_num(upper 8bit) / beats_denom(low 8 bit)
    unsigned short int freq = 1;
    // unsigned short int pwm =0;
    unsigned char note;
    unsigned char vol = 64;
};


class faust_oscillator : public zzub::port_facade_plugin {
public:
    faust_oscillator(
        const faust_oscillator_info* info
    )
      : info(info)
      , lfo("lfo", zzub::port_flow::output)
    {
        faust_lib_dir = std::string(getenv("NEIL_BASE_PATH")) + "/lib/faustlibraries";
        output = new float[zzub_buffer_size];
        phase = new float[zzub_buffer_size];
    }

    virtual ~faust_oscillator()
    {
        delete output;
    }

    // virtual void init(zzub::archive *arc, zzub::init_config* init_config) override;
    virtual void init(zzub::archive* arc) override;

    virtual void process_cv(int numsamples) override;
    virtual void process_events() override;

private:
    float* phase;
    float* output;

    float calculate_freq(unsigned short int freq, unsigned char freq_type) const;

    const faust_oscillator_info* info;
    llvm_dsp_factory* factories[5]; // builders for the sin,tri,saw,sqr,phasor faust computers
    llvm_dsp* dsp[5]; // the faust computers

    std::string faust_lib_dir;

    // the name of the faust widgets in the phasor computer - not the zzub widgets
    static inline int phase_widget = 0;

    std::vector<faust_widget_info*> faust_widgets;

    zzub_plugin_t* metaplugin;

    // the faust ui here is not a ui - it piggybacks on zzub to feed
    // parameter data to the faust computers
    faust_ui ui {};

    gvals state {};
    gvals gval {};

    cv_data_port lfo;
};


struct faust_oscillator_collection : zzub::plugincollection {
    faust_oscillator_info info {};

    virtual void initialize(zzub::pluginfactory* factory)
    {
        factory->register_info(&info);
    }
};


zzub::plugincollection* zzub_get_plugincollection()
{
    return new faust_oscillator_collection();
}
