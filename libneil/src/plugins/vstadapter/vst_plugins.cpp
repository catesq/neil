#include <cstdlib>

#include "vst_plugins.h"
#include "vst_adapter.h"


vst_plugins::vst_plugins(const char* vst_path) 
{
    this->vst_path = vst_path;
}


void 
vst_plugins::initialize(zzub::pluginfactory *factory) 
{
    for(auto plugin_info: vst_plugin_file_finder(vst_path)) {
        factory->register_info(plugin_info);
        printf("registered vst2 plugin: %s\n", plugin_info->name.c_str());
    }
}


const 
zzub::info *vst_plugins::get_info(const char *uri, zzub::archive *data) 
{
    return 0;
}


const 
char *vst_plugins::get_uri() 
{
    return 0;
}


void 
vst_plugins::configure(const char *key, const char *value) 
{

}


void 
vst_plugins::destroy() 
{
    delete this;
}


zzub::plugincollection *
zzub_get_plugincollection() 
{
    const char* envpath = std::getenv("VST_PATH");

    if(envpath == NULL)
        envpath = "vst:.vst";

    return new vst_plugins(envpath);
}


const char *
zzub_get_signature() 
{ 
    return ZZUB_SIGNATURE; 
}
