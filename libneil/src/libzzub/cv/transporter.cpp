#include "libzzub/cv/transporter.h"
#include <algorithm>

namespace zzub {



void 
cv_transporter::work(
    zzub::metaplugin& from_plugin, 
    zzub::metaplugin& to_plugin, 
    int numsamples,
    bool use_curr_work
) 
{
    if(!data || !target)
        return;

    data->reset(numsamples);
    
    for(auto source: sources)
        source->work(numsamples);

    target->send(numsamples);
}


int 
cv_transporter::get_index(
    const cv_connector& link
)
{
    for(int i = 0; i < sources.size(); i++) {
        if(sources[i]->node == link.source_node)
            return i;
    }

    return -1;
}


void
cv_transporter::move_source(
    int from_index,
    int to_index
)
{
    auto source = sources[from_index];
    sources.erase(sources.begin() + from_index);
    sources.insert(sources.begin() + to_index, source);
}


void 
cv_transporter::add_source(
    const cv_connector& link, 
    zzub::metaplugin& from,
    zzub::metaplugin& to
)
{
    if(!target) {
        target = cv_target::create(data, link.target_node, link.opts);
        target->initialize(to);
    }

    if(target && data) {
        auto source = cv_source::create(data, link.source_node, link.opts);
        source->initialize(from);
        sources.push_back(source);
    }
}


void 
cv_transporter::remove_source(
    const cv_connector& link
)
{
    for(auto it = sources.begin(); it != sources.end(); it++) {
        if((*it)->node == link.source_node) {
            sources.erase(it);
        }
    }

    if(sources.size() == 0) {
        delete data;
        delete target;
    }
}






}