#pragma once


#include "libzzub/cv/node.h"

/*************************************************************************
 *
 * cv_connector_opts
 *
 ************************************************************************/
namespace zzub
{

enum modulation_mode {
    modulation_add = 0,
    modulation_subtract = 1,
    modulation_multiply = 2,
    modulation_divide = 3,
    modulation_max = 4,
    modulation_min = 5,
    modulation_average = 6,
    modulation_assign = 7
};


struct cv_connector_opts {
    float amp = 1.0f;
    uint32_t modulate_mode = 0;
    float offset_before = 0.f;
    float offset_after = 0.f;
};

/*************************************************************************
 *
 * cv connector
 * 
 * data about a link between two ports:
 *    target node, source node, some options like volume
 *
 ************************************************************************/


struct cv_connector 
{
    cv_node source_node;
    cv_node target_node;
    cv_connector_opts opts;

    cv_connector(cv_node source, cv_node target)
    : cv_connector(source, target, cv_connector_opts{}) 
    {
    }

    cv_connector(cv_node source, cv_node target, cv_connector_opts opts)
    : source_node(source), target_node(target), opts(opts)
    {

    }

    bool operator==(const cv_connector& other) const
    { 
        return source_node == other.source_node && target_node == other.target_node; 
    }
};

}