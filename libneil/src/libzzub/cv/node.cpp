#include "libzzub/cv/node.h"

// Ensure the correct enum is included
namespace zzub {


std::string describe_cv_node(metaplugin* mplugin, const cv_node* node) 
{
    // Cast node->port_type to port_type if necessary
    std::string name = mplugin->name;

    switch (static_cast<port_type>(node->port_type)) {
        case port_type::audio:
            if (node->value == 0) {
                return name + " - audio - left";
            } else if (node->value == 1) {
                return name + " - audio - right";
            } else {
                return name + " - audio - channel " + std::to_string(node->value);
            }

        case port_type::param:
            return name + " - param - " + mplugin->plugin->describe_param(node->value);

        case port_type::track:
            return name + " - track - " + std::to_string(node->value >> 16) + " - param - " + std::to_string(node->value & 0xFFFF);

        case port_type::cv: {
            auto port = mplugin->plugin->get_port(node->value);

            if (port)
                return name + " - port - " + port->get_name();
            else
                return name + " - port - " + std::to_string(node->value);
        }

        default:
            return "";
    }

}

}
