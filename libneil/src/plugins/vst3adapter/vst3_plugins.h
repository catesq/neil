#pragma once

#include "vst3_info.h"

#include "zzub/plugin.h"
#include "zzub/signature.h"
#include <public.sdk/source/vst/hosting/hostclasses.h>



struct Vst3PluginCollection : zzub::plugincollection {
    Vst3PluginCollection(const char* vst_path);
    virtual void initialize(zzub::pluginfactory *factory);
    virtual const zzub::info *get_info(const char *uri, zzub::archive *data); // { return 0; }
    virtual const char *get_uri(); // { return 0; }
    virtual void configure(const char *key, const char *value); // {}
    virtual void destroy();

private:
    const char* vst_path = nullptr;
    Steinberg::Vst::HostApplication *standardPluginContext = nullptr;
};



