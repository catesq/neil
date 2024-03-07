#ifndef LIBNEIL_DRIVER_PORTAUDIO_H
#define LIBNEIL_DRIVER_PORTAUDIO_H

#include "driver.h"
#include <portaudio.h>
#include <string>

namespace zzub {


struct audiodriver_portaudio : audiodriver {


    zzub::timer timer;								// hires timer, for cpu-meter
    double last_work_time;							// length of last WorkStereo
    double cpu_load;
    PaStream *stream;

    audiodriver_portaudio();

    virtual ~audiodriver_portaudio() noexcept(false);
    virtual void initialize(audioworker *worker);
    virtual bool enable(bool e);

    virtual int getDeviceCount();
    virtual bool createDevice(int outputIndex, int inputIndex);
    virtual void destroyDevice();
    virtual int getDeviceByName(const char* name);
    audiodevice* getDeviceInfo(int index);
    double getCpuLoad();
};

}

#endif // LIBNEIL_DRIVER_PORTAUDIO_H
