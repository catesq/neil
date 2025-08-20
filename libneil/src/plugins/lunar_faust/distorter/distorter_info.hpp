#pragma once

#include <zzub/plugin.h>




struct dual_distorter_info : zzub::info 
{
    dual_distorter_info();

    zzub::parameter *clip_type;    // sat, hard clip, soft clip 
    zzub::parameter *clip_target;  // pos/neg/both
    zzub::parameter *thres;        // threshold
    zzub::parameter *amp;          // some distortion types use this 
    zzub::parameter *normalise;    // if clipped amplify

    virtual zzub::plugin* create_plugin() const override;
    
    virtual bool store_info(zzub::archive *data) const { return false; }
};

