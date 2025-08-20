#pragma once


#include <zzub/plugin.h>



struct cv_oscillator_info : zzub::info 
{
    cv_oscillator_info();

    zzub::parameter *wave_type;  // sin,tri,saw,sqr
    zzub::parameter *freq_type;
    zzub::parameter *freq;
    // zzub::parameter *pwm;
    zzub::parameter *note;
    zzub::parameter *vol;

    virtual zzub::plugin* create_plugin() const override;
    
    virtual bool store_info(zzub::archive *data) const { return false; }
};


