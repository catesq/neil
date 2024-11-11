#pragma once

// #include <sys/types.h>

// #include "libzzub/metaplugin.h"
// #include "libzzub/cv/base.h"
// #include "libzzub/cv/connector.h"


namespace zzub {


// dummy structs for the data_transfer;
// struct cv_connector;
// struct cv_source;



/***********************************************************************************************************
 *
 * cv_data_source - used by the cv_connector class to transfer data from source
 *
 * the data is either audio stream or plugin parameter
 **********************************************************************************************************/




/*
 * builders for the cv_input and cv_output - used in cv_connector::connected
 */

// std::shared_ptr<cv_data_source> build_cv_data_source(
//     zzub::metaplugin& from_plugin,
//     const cv_node& source,
//     const cv_connector_opts& opts
// );


// std::shared_ptr<cv_data_target> build_cv_data_target(
//     zzub::metaplugin& from_plugin,
//     const cv_node& target,
//     const cv_connector_opts& opts,
//     cv_data_source* data_source
// );


}