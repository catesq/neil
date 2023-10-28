#pragma once

#include "driver.h"
#include "timer.h"
#include <string>

// namespace zzub {


struct audiodriver_rainout : zzub::audiodriver {
    zzub::timer timer;								// hires timer, for cpu-meter
    double last_work_time;							// length of last WorkStereo
    double cpu_load;

    audiodriver_rainout();

    virtual ~audiodriver_rainout();
    virtual void initialize(zzub::audioworker *worker);
    virtual bool enable(bool e);

    virtual int getDeviceCount();
    virtual bool createDevice(int outputIndex, int inputIndex);
    virtual void destroyDevice();
    virtual int getDeviceByName(const char* name);
    zzub::audiodevice* getDeviceInfo(int index);
    double getCpuLoad();
};


