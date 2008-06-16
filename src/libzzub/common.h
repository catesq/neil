/*
Copyright (C) 2003-2007 Anders Ervik <calvin@countzero.no>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once
#if !defined(__GNUC__)
#pragma warning (disable:4786)	// Disable VC6 long name warning for some STL objects
#endif

#if defined(_WIN32)

#if defined(_DEBUG)
#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <windows.h>

#elif defined(POSIX) // 

#include <errno.h> 
#include <pthread.h>
#include <unistd.h> 
#include <semaphore.h> 

#if defined(__powerpc__) || defined(__POWERPC__) || defined(_M_PPC) || defined(__BIG_ENDIAN__)
#define ZZUB_BIG_ENDIAN
#endif

#define strcmpi strcasecmp

#endif

#ifdef min
	#undef min
#endif

#ifdef max
	#undef max
#endif

#define _USE_MATH_DEFINES
#include <cmath>
#include <cassert>
#include <cstring>
#include <map>
#include <list>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/depth_first_search.hpp>

#define ZZUB_NO_CTYPES

struct zzub_flatapi_player;

namespace zzub {
	struct metaplugin_proxy;
	struct info;
	struct pattern;
	struct event_connection_binding;
	struct wave_info_ex;
	struct wave_level;
	struct parameter;
	struct attribute;
	struct envelope_entry;
	struct midimapping;
	struct recorder;
	struct pluginlib;
	struct mem_archive;
	struct audiodriver;
	struct mididriver;
	struct instream;
	struct outstream;
};

// internal types
typedef zzub_flatapi_player zzub_player_t;
typedef zzub::audiodriver zzub_audiodriver_t;
typedef zzub::mididriver zzub_mididriver_t;
typedef zzub::metaplugin_proxy zzub_plugin_t;
typedef const zzub::info zzub_pluginloader_t;
typedef zzub::pluginlib zzub_plugincollection_t;
typedef zzub::pattern zzub_pattern_t;
typedef zzub::event_connection_binding zzub_event_connection_binding_t;
typedef zzub::wave_info_ex zzub_wave_t;
typedef zzub::wave_level zzub_wavelevel_t;
typedef zzub::parameter zzub_parameter_t;
typedef zzub::attribute zzub_attribute_t;
typedef zzub::envelope_entry zzub_envelope_t;
typedef zzub::midimapping zzub_midimapping_t;
typedef zzub::recorder zzub_recorder_t;
typedef zzub::mem_archive zzub_archive_t;
typedef zzub::instream zzub_input_t;
typedef zzub::outstream zzub_output_t;


#include "synchronization.h"
#include "zzub/plugin.h"
#include "pluginloader.h"
#include "timer.h"
#include "driver.h"
#include "midi.h"
#include "wavetable.h"
#include "input.h"
#include "output.h"
#include "master.h"
#include "recorder.h"
#include "graph.h"
#include "song.h"
#include "undo.h"
#include "operations.h"
#include "player.h"
#include "connections.h"

