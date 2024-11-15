#pragma once


#include <vector>
#include <bit>
#include <cstring>
#include <concepts>


#include "zzub/plugin.h"


namespace zzub {


/**
 * for audio rate cv stream IO
 */
template<class B>
requires(std::derived_from<B, port_buffer>)
class buffer_port : public zzub::port {
    B buffer;
    
    std::string port_name;

    const zzub::port_flow direction;

    // mostly unused but here for plugins to use in plugin::connect_ports()
    // the lfo ports in mcp_chorus/MCPChorus.cpp use it to track when the optional lfo
    // signal is connected
    // the master_plugin uses it to track when a audio signal is attached
    // to the multi channel recorder ports
    bool connected;

public:
    /**
     * constructor
     * 
     * @param name - name of the port
     * @param direction - input or output port
     */
    buffer_port(
        std::string name,
        zzub::port_flow direction
    ) : buffer_port(name, direction, zzub_buffer_size)
    {
    }

    /**
     * constructor
     * 
     * @param name - name of the port
     * @param direction - input or output port
     */
    buffer_port(
        std::string name,
        zzub::port_flow direction,
        uint bufsize
    )
        : port_name(name)
        , direction(direction)
        , buffer(bufsize)
    {
    }


    virtual ~buffer_port() {}
    

    /**
     * @return name of the port
     */
    virtual const char* get_name() override
    {
        return port_name.c_str();
    }


    /**
     * @return input or output
     */
    virtual zzub::port_flow get_flow() override
    {
        return direction;
    };


    /**
     * @return zzub::port_type::cv
     */
    virtual zzub::port_type get_type() override
    {
        return zzub::port_type::cv;
    };


    /**
     * @param buf - destination buffer
     * @param count - number of samples to write to buffer
     * @param use_delay_frame - use previous or current audio frame
     */
    virtual void get_value(float* buf, uint count, bool use_delay_frame) override
    {
        buffer.read(buf, count, use_delay_frame);
    }


    /**
     * @param buf - buffer to read from
     * @param count - number of samples to read
     */
    virtual void set_value(
        float* buf,
        uint count
    ) override
    {
        buffer.write(buf, count);
    }
};



}