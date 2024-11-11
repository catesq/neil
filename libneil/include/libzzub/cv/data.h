#pragma once


#include <vector>

#include "libzzub/cv/node.h"
#include "libzzub/metaplugin.h"
#include "zzub/plugin.h"

namespace zzub 
{


    
/**
 */
enum class cv_data_type {
    /**
     * stereo audio 
     * zzub_audio currently has four buffers which are zzub_buffer_size long
     *   two buffers for the current frame of stereo audio
     *   two feedback buffers have copies from the previous frame
     */
    zzub_audio,

    /**
     * the old internal zzub
     */
    zzub_param,

    /**
     * a float value
     * uses zzub::port::get_value()
     */
    cv_param,

    /**
     * mono audio
     * main difference from zzub_audio data is how zzub audio is stored in metaplugin
     */
    cv_stream

};


cv_data_type get_data_type(zzub::metaplugin& mp, const cv_node& node, zzub::port_flow flow);


/**
 * cv_data 
 * 
 * data to be sent to target cv port, may be the sum of several source ports
 * 
 * each target port in the target plugin has one a cv_data_transporter 
 * that manages a single data target 
 * the data target is shared by one or more source_data_merge objects
 * to accumulate the cv value of the current frame
 */

struct cv_data {
    cv_data_type data_type;

    cv_data(cv_data_type data_type)
     : data_type(data_type) {}

    virtual void reset(int numsamples) = 0;

    virtual void add(float value) = 0;
    virtual void add(float* value, int numsamples) = 0;

    virtual float get() = 0;
    virtual float* get(int numsamples) = 0;

    static cv_data* create(cv_data_type data_type);
};



/**
 * cv_data_param
 * 
 * a single float value, used by cv_target_zzub_param cv_target_port_param
 */
struct cv_data_param : public cv_data {
    cv_data_param(cv_data_type data_type)
     : cv_data(data_type) {}

    virtual void reset(int numsamples) override;

    virtual void add(float value) override;
    virtual void add(float* value, int numsamples) override;

    virtual float get() override;
    virtual float* get(int numsamples) override;
protected:
    float value;
};



/**
 * cv_data_stream
 * 
 * 
 */
struct cv_data_stream : public cv_data {
    cv_data_stream(cv_data_type data_type) 
    : cv_data(data_type), 
      data(zzub_buffer_size, 0) 
    {}

    virtual void reset(int numsamples) override;

    virtual void add(float value) override;
    virtual void add(float* value, int numsamples) override;

    virtual float get() override;
    virtual float* get(int numsamples) override;

protected:
    std::vector<float> data;
};


}