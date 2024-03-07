#pragma once

#include <sys/types.h>

#include "libzzub/metaplugin.h"

namespace zzub {


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *


enum cv_node_type {
    // the cv_node_type - stored in cv_node.type - changes how the cv_node.value is interpreted
    audio_node = 1,         // value will be 0 or 1 for L/R audio channel
    zzub_param_node = 2,    // value: index of a zzub plugin parameter
    ext_port_node  = 3,     // value: index of a external plugin port - lv2/vst2 or 3
    midi_track_node  = 4, // value: copy notes/volume to + from tracks of zzub plugin
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *


// enum cv_node_flags {
//     audio_in = 1,   // read audio from input

//     audio_out = 2,  // read audio from output 

//     midi_track = 128,  // zzub::midi_track or 

//     midi_seq = 256,   // raw midi
// };


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *


// when node type is audio:
//   input/output channel determined by whether it's the souce or target of the cv_node in cv_port_link
//   channel is 0 or 1 -> the left or right channel
// when node_type is value or stream:
//   param is the index of the zzub_parameter in zzub_plugins globals



struct cv_node {
    int plugin_id = -1;

    int type;

    int value;

    bool operator==(const cv_node& other) const { return plugin_id == other.plugin_id && type == other.type && value == other.value; }
};



// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *


// cv_input and cv_output are used in the work method of cv_connector to move data
// from source to target plugin
// for of time the data will be a either:
//   a single constant value
//   a stream of 32bit floating point data at the audio rate - for a mono audio channel or cv data
//   8 bit raw midi stream
//   some zzub structure - probbly midi_track data 


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

// cv output reads data to source plugin



struct cv_input {
    cv_node node;
    
    cv_input(const cv_node& source) : node(source) {}

    virtual bool read(zzub::metaplugin& source, zzub::metaplugin& target, int numsamples) = 0;
};




struct cv_input_audio : public cv_input {
    float data[256];
    
    using cv_input::cv_input;

    virtual bool read(zzub::metaplugin& source, zzub::metaplugin& target, int numsamples);
};



struct cv_input_zzub_param : public cv_input  {
    int i;

    using cv_input::cv_input;

    virtual bool read(zzub::metaplugin& source, zzub::metaplugin& target, int numsamples);
};



struct cv_input_ext_port : public cv_input  {
    union {
        float f;
        int i;
    } value;

    using cv_input::cv_input;

    virtual bool read(zzub::metaplugin& source, zzub::metaplugin& target, int numsamples);
};



struct cv_input_midi : cv_input {
    using cv_input::cv_input;

    unsigned char data[2048];
    uint len;

    virtual bool read(zzub::metaplugin& source, zzub::metaplugin& target, int numsamples);
};



// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

// cv output writes data to target plugi


struct cv_output {
    cv_node node;
    cv_output(const cv_node& target) : node(target) {}

    virtual bool write(cv_input* read, zzub::metaplugin& source, zzub::metaplugin& target, int numsamples) = 0;
};



struct cv_output_zzub_param : cv_output {
    using cv_output::cv_output;

    virtual bool write(cv_input* read, zzub::metaplugin& source, zzub::metaplugin& target, int numsamples);
};



struct cv_output_ext_port : cv_output{
    using cv_output::cv_output;

    virtual bool write(cv_input* read, zzub::metaplugin& source, zzub::metaplugin& target, int numsamples);
};



struct cv_output_midi_track : cv_output{
    using cv_output::cv_output;

    virtual bool write(cv_input* read, zzub::metaplugin& source, zzub::metaplugin& target, int numsamples);
};



struct cv_output_audio  : cv_output {
    using cv_output::cv_output;

    // some types of cv_input need a buffer to copy a single value up to 256 times  
    float buf[256];

    virtual bool write(cv_input* read, zzub::metaplugin& source, zzub::metaplugin& target, int numsamples);
    virtual bool write_buffer(float* data, zzub::metaplugin& source, zzub::metaplugin& target, int numsamples);
};



// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *


std::shared_ptr<cv_input> build_cv_input(const cv_node& source);

std::shared_ptr<cv_output> build_cv_output(const cv_node& target);



}