#include <zzub/zzub.h>
#include <zzub/signature.h>
#include <zzub/plugin.h>
#include "faust/dsp/llvm-dsp.h"
#include "faust_ui.hpp"

const char *zzub_get_signature()
{
    return ZZUB_SIGNATURE;
}



class gvals 
{
public:
    unsigned char wave_type; // sin,tri,saw,sqr
    unsigned char freq_type; // Hz, milliHz, beats, beats/256
    unsigned short int freq;
    unsigned short int pwm;
    unsigned char note;
    unsigned char vol;
};



class faust_oscillator : public zzub::plugin 
{
public:
    faust_oscillator() {}

    virtual ~faust_oscillator() { }

    virtual void init(zzub::archive *arc) override;

    virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode) override;

private:    
    
    llvm_dsp_factory *factories[4];  //sin,tri,saw,sqr, phasor
    llvm_dsp* dsp[5];               
    faust_ui ui{};
    gvals gval;
};



struct faust_oscillator_info : zzub::info 
{
    faust_oscillator_info();

    zzub::parameter *wave_type;  // sin,tri,saw,sqr
    zzub::parameter *freq_type;
    zzub::parameter *freq;
    zzub::parameter *pwm;
    zzub::parameter *note;
    zzub::parameter *vol;

    virtual zzub::plugin* create_plugin() const 
    { 
        return new faust_oscillator();
    }
    

    virtual bool store_info(zzub::archive *data) const 
    { 
        return false; 
    }
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
