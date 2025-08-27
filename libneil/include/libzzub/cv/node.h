#pragma once

#include "zzub/plugin.h"
#include "libzzub/metaplugin.h"

namespace zzub
{

/**
 * @brief Generates a textual description of a CV (control voltage) node.
 * 
 * @param mplugin Pointer to the metaplugin 
 * @param node Pointer to the CV node 
 * @return std::string A human-readable description of the CV node.
 */
std::string describe_cv_node(metaplugin* mplugin, const cv_node* node);

}
