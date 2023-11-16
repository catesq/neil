#pragma once

struct zzub_flatapi_player;

namespace zzub {
	struct audiodriver;
	struct mididriver;
	struct metaplugin_proxy;
	struct info;
	struct pluginlib;
	struct pattern;
	struct sequence_proxy ;
	struct wave_proxy ;
	struct wavelevel_proxy;
	struct parameter;
	struct attribute;
	struct envelope_entry;
	struct midimapping;
	struct recorder;
	struct mem_archive;
	struct instream;
	struct outstream;
	struct cv_port_link;
	struct connection;
	struct event_connection_binding ;
	struct event_connection;
	struct cv_connection;
}

typedef zzub_flatapi_player zzub_player_t;
typedef zzub::audiodriver zzub_audiodriver_t;
typedef zzub::mididriver zzub_mididriver_t;
typedef zzub::metaplugin_proxy zzub_plugin_t;
typedef const zzub::info zzub_pluginloader_t;
typedef zzub::pluginlib zzub_plugincollection_t;
typedef zzub::pattern zzub_pattern_t;
typedef zzub::sequence_proxy zzub_sequence_t;
typedef zzub::event_connection_binding zzub_event_connection_binding_t;
typedef zzub::connection zzub_connection_t;
typedef zzub::cv_connection zzub_cv_connection_t;
typedef zzub::event_connection zzub_event_connection_t;

typedef zzub::cv_port_link zzub_cv_port_link_t;
typedef zzub::wave_proxy zzub_wave_t;
typedef zzub::wavelevel_proxy zzub_wavelevel_t;
typedef zzub::parameter zzub_parameter_t;
typedef zzub::attribute zzub_attribute_t;
typedef zzub::envelope_entry zzub_envelope_t;
typedef zzub::midimapping zzub_midimapping_t;
typedef zzub::recorder zzub_recorder_t;
typedef zzub::mem_archive zzub_archive_t;
typedef zzub::instream zzub_input_t;
typedef zzub::outstream zzub_output_t;
