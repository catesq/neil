#pragma once

#include <format>

#include "libzzub/host.h"
#include "zzub/plugin.h"


namespace zzub
{

/**
 * this is a placeholder so audio ports get an entry in port lists
 * 
 * the audio data copying is in:
 *   libzzub/cv/source  libzzub/cv/target  libzzub/connections  
 */
class audio_port : public zzub::port
{
    zzub::host* host;
    zzub::port_flow flow;
    std::string name;
    int channel;

public:
    audio_port(
        zzub::host* host, 
        zzub::port_flow port_flow,
        std::string fmt_name,
        int channel
    ) : host(host)
      , flow(port_flow)
      , name(std::vformat(fmt_name, std::make_format_args(channel)))
      , channel(channel)
    {
    }

    //
    zzub::port_flow
    get_flow() 
    {
        return flow;
    };


    //
    const char*
    get_name() 
    {
        return name.c_str();
    };


    //
    zzub::port_type
    get_type() override 
    {
        return zzub::port_type::audio;
    }


};

}