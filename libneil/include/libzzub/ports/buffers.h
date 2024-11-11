#pragma once

#include "zzub/plugin.h"
#include <vector>

namespace zzub
{


struct port_buffer {
    virtual void read(float* dest, uint num, bool delay_frame) = 0;
    virtual void write(float* src, uint num) = 0;
};


/**
 * The delay frame argument in read() is ignored
 */
class basic_buf : public port_buffer {
    uint block_size;
    std::vector<float> data;

public:
    basic_buf() : basic_buf(zzub_buffer_size)
    {
    }


    basic_buf(uint block_size) 
    : block_size(block_size),
      data(block_size, 0)
    {
    }


    virtual void read(float* dest, uint num, bool delay_frame);

    virtual void write(float* src, uint num);
};




/**
 * A ringbuffer twice the size of the blocks of samples used by zzub
 * Depending on the location of plugins in the work order it
 * reads audio x samples before the write index where:
 *    x = block size       (when delay_frame=False)
 *    x = block size * 2   (when delay_frame=True) 
 * 
 * This is because the ringbuffer may be used by 
 * plugins earlier or later in the work order
 * The delay_frame var is so they get the same output 
 */
class basic_rb : public port_buffer {
    uint block_size;

    // buffer size
    uint buf_size;

    // mask for the buffer size
    uint bufmask;

    // samples
    std::vector<float> data;

    // write index
    uint wi;

public:
    /**
     * constructor
     */
    basic_rb() : basic_rb(zzub_buffer_size)
    {
    }


    basic_rb(uint block_size)
        : block_size(block_size),
          buf_size(2 * block_size), 
          bufmask(buf_size - 1), 
          data(buf_size, 0),  
          wi(0)
    {
    }


    /* read from the buffer
     *
     * @param dest - destination buffer, must be zzub_buffer_size long
     * @param num - number of samples to read, up to zzub_buffer_size
     * @param delay_frame - read from 
     * @param delay_frame - whether to delay by zzub_buffer_size samples
     */
    virtual void read(float* dest, uint num, bool delay_frame);


    /**
     *  write to the buffer
     * 
     * @param src - source buffer
     * @param num - number of samples to write
     */
    virtual void write(float* src, uint num);
};





}