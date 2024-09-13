#include <zzub/zzub.h>
#include <zzub/signature.h>
#include <zzub/plugin.h>

#include "faust/dsp/llvm-dsp.h"
#include "faust_ui.hpp"

#include "faust_oscillator_info.hpp"



const char *zzub_get_signature()
{
    return ZZUB_SIGNATURE;
}




class gvals 
{
public:
    unsigned char wave_type = 0; // sin,tri,saw,sqr
    unsigned char freq_type = 0; // Hz, milliHz, beats, beats/192, beats_num(upper 8bit) / beats_denom(low 8 bit)
    unsigned short int freq = 1;
    // unsigned short int pwm =0;
    unsigned char note;
    unsigned char vol = 64;
};




class faust_oscillator : public zzub::plugin 
{
public:
    faust_oscillator(const faust_oscillator_info* info) : info(info) 
    {
        faust_lib_dir = std::string(getenv("NEIL_BASE_PATH")) + "/lib/faustlibraries";
    }

    virtual ~faust_oscillator() { }

    // virtual void init(zzub::archive *arc, zzub::init_config* init_config) override;
    virtual void init(zzub::archive *arc) override;

    bool process_mono(float **pin, float **pout, int numsamples, int mode);
    virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode) override;
    virtual void process_events() override;

private:

    float calculate_freq(unsigned short int freq, unsigned char freq_type) const;

    const faust_oscillator_info* info;
    llvm_dsp_factory *factories[5];  //sin,tri,saw,sqr, phasor
    llvm_dsp* dsp[5];

    std::string faust_lib_dir;

    // the name of the faust widgets in the phasor computer - not the zzub widgets
    static inline int phase_widget = 0;

    std::vector<faust_widget_info*> faust_widgets;

    faust_ui ui{};
    gvals state{};
    gvals gval{};
};




struct faust_oscillator_collection : zzub::plugincollection 
{
    faust_oscillator_info info {};

    virtual void initialize(zzub::pluginfactory *factory) 
    {
        factory->register_info(&info);
    }
};



zzub::plugincollection *zzub_get_plugincollection() 
{
    return new faust_oscillator_collection();
}
