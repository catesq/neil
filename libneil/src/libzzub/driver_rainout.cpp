#include "libzzub/driver_rainout.h"


audiodriver_rainout::audiodriver_rainout() {
}


audiodriver_rainout::~audiodriver_rainout() {
}


void audiodriver_rainout::initialize(zzub::audioworker *worker) {
}


bool audiodriver_rainout::enable(bool e) {
    return false;
}


int audiodriver_rainout::getDeviceCount() {
    return 1;
}


bool audiodriver_rainout::createDevice(int outputIndex, int inputIndex) {
    return false;
}


void audiodriver_rainout::destroyDevice() {
}


int audiodriver_rainout::getDeviceByName(const char* name) {
    return 0;
}


zzub::audiodevice* audiodriver_rainout::getDeviceInfo(int index) {
    return nullptr;
}


double audiodriver_rainout::getCpuLoad() {
    return 0.f;
}