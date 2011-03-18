#if !defined(LUNAR_PLUGIN_LOCALS_H)
#define LUNAR_PLUGIN_LOCALS_H
#define LUNAR_USE_LOCALS
#if defined(__cplusplus)
extern "C" {
#endif // __cplusplus
typedef struct _lunar_attributes {
} lunar_attributes_t;
typedef struct _lunar_globals {
	float *drywet;
	float *feedback;
	float *lfo_min;
	float *lfo_max;
	float *lfo_rate;
	float *lfo_phase;
	float *stages;
} lunar_globals_t;
typedef struct _lunar_global_values {
	float drywet;
	float feedback;
	float lfo_min;
	float lfo_max;
	float lfo_rate;
	float lfo_phase;
	float stages;
} lunar_global_values_t;
typedef struct _lunar_track {
} lunar_track_t;
typedef struct _lunar_track_values {
} lunar_track_values_t;
typedef struct _lunar_controllers {
} lunar_controllers_t;
typedef struct _lunar_controller_values {
} lunar_controller_values_t;
#if defined(__cplusplus)
}
#endif // __cplusplus
#endif // LUNAR_PLUGIN_LOCALS_H