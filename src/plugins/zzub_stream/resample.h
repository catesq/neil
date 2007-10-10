#pragma once

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

#include "def.h"
#include "Downsampler2Flt.h"
#include "fnc.h"
#include "Fixed3232.h"
#include "Int16.h"
#include "Int64.h"
#include "InterpFlt.h"
#include "InterpPack.h"
#include "MipMapFlt.h"
#include "ResamplerFlt.h"
#include "StopWatch.h"

struct resampler {
	bool initialized;
	rspl::InterpPack interp_pack;
	rspl::MipMapFlt mip_map;
	rspl::ResamplerFlt	rspl;
	resampler();
	void init(float* samples, int numsamples);
};

struct stereo_resampler {
	resampler resampleL, resampleR;
	void init(float* samplesL, float* samplesR, int numsamples);
	void interpolate_block(float* samplesL, float* samplesR, int numsamples);
	void set_pitch(long pitch);
};

struct stream_resampler {
	enum {
		overlap_samples = 256,
		resampler_samples = 4096*4,
		max_samples_per_tick = resampler_samples * (1 << 6), // hold sampledata for up to 6 octaves more than base note
	};

	zzub::plugin* plugin;
	bool playing;
	int note;
	int stream_sample_rate;
	int stream_base_note;
	int samples_in_resampler;
	bool first_fill;
	int next_fill_overlap;
	int fade_pos;

	stereo_resampler resample;
	float bufferL[max_samples_per_tick];
	float bufferR[max_samples_per_tick];

	float remainderL[overlap_samples];
	float remainderR[overlap_samples];

	stream_resampler(zzub::plugin* plugin);

	void fill_resampler();
	void set_pitch(long pitch);
	bool process_stereo(float** pout, int numsamples);
	void set_stream_pos(unsigned int ofs);

	void crossfade(float** pout, int numsamples);
};
