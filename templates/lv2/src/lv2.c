/*
 * Tibia
 *
 * Copyright (C) 2024, 2025 Orastron Srl unipersonale
 *
 * Tibia is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Tibia is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Tibia.  If not, see <http://www.gnu.org/licenses/>.
 *
 * File author: Stefano D'Angelo
 */

#include <stdlib.h>
#include <stdint.h>

#include "data.h"
#include "plugin_api.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include "plugin.h"
#ifdef DATA_UI
# include "plugin_ui.h"
#endif
#pragma GCC diagnostic pop

#include <lv2/core/lv2.h>
#include <lv2/core/lv2_util.h>
#include <lv2/log/log.h>
#include <lv2/log/logger.h>
#include <lv2/urid/urid.h>
#if (DATA_PRODUCT_MIDI_INPUTS_N + DATA_PRODUCT_MIDI_OUTPUTS_N > 0) || defined(DATA_STATE_DSP_CUSTOM)
# include <lv2/atom/atom.h>
# if DATA_PRODUCT_MIDI_INPUTS_N + DATA_PRODUCT_MIDI_OUTPUTS_N > 0
#  include <lv2/atom/util.h>
#  include <lv2/midi/midi.h>
# endif
#endif
#ifdef DATA_UI
# include <lv2/ui/ui.h>
#endif
#ifdef DATA_STATE_DSP_CUSTOM
# include <lv2/state/state.h>
#endif
#ifdef DATA_MESSAGING
#include <lv2/atom/forge.h>
#endif
#include <string.h>

#if defined(__i386__) || defined(__x86_64__)
# include <xmmintrin.h>
# include <pmmintrin.h>
#endif

#if (DATA_PRODUCT_CONTROL_INPUTS_N > 0) && defined(DATA_STATE_DSP_CUSTOM)
# include <stdatomic.h>
# if defined(_WIN32) || defined(__CYGWIN__)
#  include <processthreadsapi.h>
#  define yield SwitchToThread
# else
#  include <sched.h>
#  define yield sched_yield
# endif
#endif

#define CONTROL_INPUT_INDEX_OFFSET ( \
		DATA_PRODUCT_AUDIO_INPUT_CHANNELS_N \
		+ DATA_PRODUCT_AUDIO_OUTPUT_CHANNELS_N \
		+ DATA_PRODUCT_MIDI_INPUTS_N \
		+ DATA_PRODUCT_MIDI_OUTPUTS_N )
#define CONTROL_OUTPUT_INDEX_OFFSET	(CONTROL_INPUT_INDEX_OFFSET + DATA_PRODUCT_CONTROL_INPUTS_N)

#if DATA_PRODUCT_CONTROL_INPUTS_N > 0
static inline float clampf(float x, float m, float M) {
	return x < m ? m : (x > M ? M : x);
}

static float adjust_param(size_t index, float value) {
	if (param_data[index].flags & DATA_PARAM_BYPASS)
		value = value > 0.f ? 0.f : 1.f;
	else if (param_data[index].flags & DATA_PARAM_TOGGLED)
		value = value > 0.f ? 1.f : 0.f;
	else if (param_data[index].flags & DATA_PARAM_INTEGER)
		value = (int32_t)(value + (value >= 0.f ? 0.5f : -0.5f));
	return clampf(value, param_data[index].min, param_data[index].max);
}
#endif

#ifdef DATA_MESSAGING
typedef struct {
	LV2_URID prop_customMessage;
	LV2_URID atom_String;
	LV2_URID atom_eventTransfer;
	LV2_URID ui_On;
	LV2_URID length;
} communication_URIs;

static inline void map_comm_uris(LV2_URID_Map* map, communication_URIs* uris) {
	uris->prop_customMessage = map->map(map->handle, "http://example.org#message");
	uris->atom_String        = map->map(map->handle, LV2_ATOM__String);
	uris->atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);
	uris->ui_On              = map->map(map->handle, DATA_LV2_URI "#UIOn");
	uris->length             = map->map(map->handle, LV2_ATOM__Int);
}

#endif

typedef struct {
	plugin				p;
	float				sample_rate;
#if DATA_PRODUCT_AUDIO_INPUT_CHANNELS_N > 0
	const float *			x[DATA_PRODUCT_AUDIO_INPUT_CHANNELS_N];
#endif
#if DATA_PRODUCT_AUDIO_OUTPUT_CHANNELS_N > 0
	float *				y[DATA_PRODUCT_AUDIO_OUTPUT_CHANNELS_N];
#endif
#if DATA_PRODUCT_MIDI_INPUTS_N > 0
	const LV2_Atom_Sequence *	x_midi[DATA_PRODUCT_MIDI_INPUTS_N];
#endif
#if DATA_PRODUCT_MIDI_OUTPUTS_N > 0
	LV2_Atom_Sequence *		y_midi[DATA_PRODUCT_MIDI_OUTPUTS_N];
#endif
#if (DATA_PRODUCT_CONTROL_INPUTS_N + DATA_PRODUCT_CONTROL_OUTPUTS_N) > 0
	float *				c[DATA_PRODUCT_CONTROL_INPUTS_N + DATA_PRODUCT_CONTROL_OUTPUTS_N];
#endif
#if DATA_PRODUCT_CONTROL_INPUTS_N > 0
	float				params[DATA_PRODUCT_CONTROL_INPUTS_N];
# ifdef DATA_STATE_DSP_CUSTOM
	float				params_sync[DATA_PRODUCT_CONTROL_INPUTS_N];
	atomic_flag			sync_lock_flag;
	char				synced;
	char				loaded;
# endif
#endif
	void *				mem;
	char *				bundle_path;
	LV2_Log_Logger			logger;
	LV2_URID_Map *			map;
#if DATA_PRODUCT_MIDI_INPUTS_N + DATA_PRODUCT_MIDI_OUTPUTS_N > 0
	LV2_URID			uri_midi_MidiEvent;
#endif
#ifdef DATA_STATE_DSP_CUSTOM
	LV2_URID			uri_atom_Chunk;
	LV2_URID			uri_state_data;
	plugin_state_callbacks		state_cbs;
	LV2_State_Store_Function	state_store;
	LV2_State_Handle		state_handle;
#endif

#ifdef DATA_MESSAGING
	LV2_Atom_Forge       forge;
	LV2_Atom_Forge_Frame frame;

	communication_URIs c_uris;
	const LV2_Atom_Sequence* control;
	LV2_Atom_Sequence*       notify;
#endif
} plugin_instance;

static const char * get_bundle_path_cb(void *handle) {
	plugin_instance *instance = (plugin_instance *)handle;
	return instance->bundle_path;
}

#ifdef DATA_STATE_DSP_CUSTOM
static void state_lock_cb(void *handle) {
	plugin_instance * i = (plugin_instance *)handle;
	while (atomic_flag_test_and_set(&i->sync_lock_flag))
		yield();
	i->synced = 0;
}

static void state_unlock_cb(void *handle) {
	plugin_instance * i = (plugin_instance *)handle;
	atomic_flag_clear(&i->sync_lock_flag);
}

static int state_write_cb(void *handle, const char *data, size_t length) {
	plugin_instance * i = (plugin_instance *)handle;
	return i->state_store(i->state_handle, i->uri_state_data, data, length, i->uri_atom_Chunk, LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
}

# if DATA_PRODUCT_CONTROL_INPUTS_N > 0
static void state_set_parameter_cb(void *handle, size_t index, float value) {
	plugin_instance * i = (plugin_instance *)handle;
	size_t idx = index_to_param[index] - CONTROL_INPUT_INDEX_OFFSET;
	value = adjust_param(idx, value);
	i->params_sync[idx] = value;
	i->loaded = 1;
}
# endif
#endif

#ifdef DATA_MESSAGING
static char send_to_ui (void *handle, const void *data, size_t bytes) {

	plugin_instance * i = (plugin_instance *) handle;

	if (!data || bytes <= 0 || bytes > DATA_MESSAGING_MAX) {
		return 1;
	}

	const uint32_t space = i->notify->atom.size;
	lv2_atom_forge_set_buffer(&i->forge, (uint8_t*)i->notify, space);
	lv2_atom_forge_sequence_head(&i->forge, &i->frame, 0);

	LV2_Atom_Forge_Frame frame;

	lv2_atom_forge_frame_time(&i->forge, 0);
	lv2_atom_forge_object(&i->forge, &frame, 0, i->c_uris.prop_customMessage);

	lv2_atom_forge_key(&i->forge, i->c_uris.length);
	lv2_atom_forge_int(&i->forge, (int32_t)bytes);

	lv2_atom_forge_key(&i->forge, i->c_uris.prop_customMessage);
	lv2_atom_forge_write(&i->forge, data, bytes);

	lv2_atom_forge_pop(&i->forge, &frame);

	return 0;
}
#endif

static LV2_Handle instantiate(const struct LV2_Descriptor * descriptor, double sample_rate, const char * bundle_path, const LV2_Feature * const * features) {
	(void)descriptor;
	(void)bundle_path;

	plugin_instance *instance = malloc(sizeof(plugin_instance));
	if (instance == NULL)
		goto err_instance;

	instance->bundle_path = strdup(bundle_path);
	if (instance->bundle_path == NULL)
		goto err_bundle_path;

	// from https://lv2plug.in/book
	const char * missing = lv2_features_query(features,
		LV2_LOG__log,	&instance->logger.log,	false,
		LV2_URID__map,	&instance->map,		true,
		NULL);

	lv2_log_logger_set_map(&instance->logger, instance->map);
	if (missing) {
		lv2_log_error(&instance->logger, "Missing feature <%s>\n", missing);
#ifdef DATA_PRODUCT_MIDI_REQUIRED
		goto err_urid;
#endif
	}

#if DATA_PRODUCT_MIDI_INPUTS_N + DATA_PRODUCT_MIDI_OUTPUTS_N > 0
	if (instance->map)
		instance->uri_midi_MidiEvent = instance->map->map(instance->map->handle, LV2_MIDI__MidiEvent);
#endif
#ifdef DATA_STATE_DSP_CUSTOM
	if (instance->map) {
		instance->uri_atom_Chunk = instance->map->map(instance->map->handle, LV2_ATOM__Chunk);
		instance->uri_state_data = instance->map->map(instance->map->handle, DATA_LV2_URI "#state_data");
	}
#endif

	plugin_callbacks cbs = {
		/* .handle		= */ (void *)instance,
		/* .format		= */ "lv2",
		/* .get_bindir		= */ get_bundle_path_cb,
		/* .get_datadir		= */ get_bundle_path_cb,
#ifdef DATA_MESSAGING
		/* .send_to_ui = */ send_to_ui
#endif
	};
	plugin_init(&instance->p, &cbs);

	instance->sample_rate = (float)sample_rate;
	plugin_set_sample_rate(&instance->p, instance->sample_rate);
	size_t req = plugin_mem_req(&instance->p);
	if (req != 0) {
		instance->mem = malloc(req);
		if (instance->mem == NULL) {
			lv2_log_error(&instance->logger, "Not enough memory\n");
			goto err_mem;
		}
		plugin_mem_set(&instance->p, instance->mem);
	} else
		instance->mem = NULL;

#if DATA_PRODUCT_AUDIO_INPUT_CHANNELS_N > 0
	for (uint32_t i = 0; i < DATA_PRODUCT_AUDIO_INPUT_CHANNELS_N; i++)
		instance->x[i] = NULL;
#endif
#if DATA_PRODUCT_AUDIO_OUTPUT_CHANNELS_N > 0
	for (uint32_t i = 0; i < DATA_PRODUCT_AUDIO_OUTPUT_CHANNELS_N; i++)
		instance->y[i] = NULL;
#endif
#if DATA_PRODUCT_MIDI_INPUTS_N > 0
	for (uint32_t i = 0; i < DATA_PRODUCT_MIDI_INPUTS_N; i++)
		instance->x_midi[i] = NULL;
#endif
#if DATA_PRODUCT_MIDI_OUTPUTS_N > 0
	for (uint32_t i = 0; i < DATA_PRODUCT_MIDI_OUTPUTS_N; i++)
		instance->y_midi[i] = NULL;
#endif
#if (DATA_PRODUCT_CONTROL_INPUTS_N + DATA_PRODUCT_CONTROL_OUTPUTS_N) > 0
	for (uint32_t i = 0; i < DATA_PRODUCT_CONTROL_INPUTS_N + DATA_PRODUCT_CONTROL_OUTPUTS_N; i++)
		instance->c[i] = NULL;
#endif
#ifdef DATA_MESSAGING
	lv2_atom_forge_init(&instance->forge, instance->map);
	map_comm_uris(instance->map, &instance->c_uris);
#endif

	return instance;

err_mem:
	plugin_fini(&instance->p);
#ifdef DATA_PRODUCT_MIDI_REQUIRED
err_urid:
#endif
	free(instance->bundle_path);
err_bundle_path:
	free(instance);
err_instance:
	return NULL;
}

static void connect_port(LV2_Handle instance, uint32_t port, void * data_location) {
	plugin_instance * i = (plugin_instance *)instance;
	printf("connect_port %u \n", port);
#ifdef DATA_MESSAGING
	// Well, these should stay last...
	if (port == DATA_MESSAGING_PORT_IN) {
		i->control = (const LV2_Atom_Sequence*)data_location;
		return;
	}
	if (port == DATA_MESSAGING_PORT_OUT) {
		i->notify = (LV2_Atom_Sequence*)data_location;
		return;
	}
#endif
#if DATA_PRODUCT_AUDIO_INPUT_CHANNELS_N > 0
	if (port < DATA_PRODUCT_AUDIO_INPUT_CHANNELS_N) {
		i->x[port] = data_location;
		return;
	}
	port -= DATA_PRODUCT_AUDIO_INPUT_CHANNELS_N;
#endif
#if DATA_PRODUCT_AUDIO_OUTPUT_CHANNELS_N > 0
	if (port < DATA_PRODUCT_AUDIO_OUTPUT_CHANNELS_N) {
		i->y[port] = data_location;
		return;
	}
	port -= DATA_PRODUCT_AUDIO_OUTPUT_CHANNELS_N;
#endif
#if DATA_PRODUCT_MIDI_INPUTS_N > 0
	if (port < DATA_PRODUCT_MIDI_INPUTS_N) {
		i->x_midi[port] = data_location;
		return;
	}
	port -= DATA_PRODUCT_MIDI_INPUTS_N;
#endif
#if DATA_PRODUCT_MIDI_OUTPUTS_N > 0
	if (port < DATA_PRODUCT_MIDI_OUTPUTS_N) {
		i->y_midi[port] = data_location;
		return;
	}
	port -= DATA_PRODUCT_MIDI_OUTPUTS_N;
#endif
#if (DATA_PRODUCT_CONTROL_INPUTS_N + DATA_PRODUCT_CONTROL_OUTPUTS_N) > 0
	i->c[port] = data_location;
#endif
}

static void activate(LV2_Handle instance) {
	plugin_instance * i = (plugin_instance *)instance;
#if DATA_PRODUCT_CONTROL_INPUTS_N > 0
	for (uint32_t j = 0; j < DATA_PRODUCT_CONTROL_INPUTS_N; j++) {
		i->params[j] = i->c[j] != NULL ? *i->c[j] : param_data[j].def;
		plugin_set_parameter(&i->p, param_data[j].index, i->params[j]);
	}
# ifdef DATA_STATE_DSP_CUSTOM
	for (uint32_t j = 0; j < DATA_PRODUCT_CONTROL_INPUTS_N; j++)
		i->params_sync[j] = i->params[j];
	// why is this not correct?
	// i->sync_lock_flag = ATOMIC_FLAG_INIT;
	atomic_flag_clear(&i->sync_lock_flag);
	i->synced = 1;
	i->loaded = 0;
# endif
#endif
	plugin_reset(&i->p);
}

static void run(LV2_Handle instance, uint32_t sample_count) {
	plugin_instance * i = (plugin_instance *)instance;

	printf("run A \n"); fflush(stdout);

	if (0 && i->control) {
		const LV2_Atom_Event* ev = lv2_atom_sequence_begin(&(i->control)->body);
		while (!lv2_atom_sequence_is_end(&i->control->body, i->control->atom.size, ev)) {
			if (lv2_atom_forge_is_object_type(&i->forge, ev->body.type)) {
				const LV2_Atom_Object* obj = (const LV2_Atom_Object*)&ev->body;

				printf("audio - received object, otype: %d\n", obj->body.otype);

				const LV2_Atom* msg_atom = NULL;

				lv2_atom_object_get(obj, i->c_uris.prop_customMessage, &msg_atom, 0);

				printf("audio - custom msg %d \n", i->c_uris.prop_customMessage);
				printf("audio - %p \n", msg_atom);
				printf("audio - %d %d \n", msg_atom->type, i->c_uris.atom_String);

				if (msg_atom && msg_atom->type == i->c_uris.atom_String) {
					const char* msg_str = (const char*)(msg_atom + 1);
					printf("audio - received string from UI: %s\n", msg_str);
				}
			}
			ev = lv2_atom_sequence_next(ev);
		}
	}


#if defined(__aarch64__)
	uint64_t fpcr;
	__asm__ __volatile__ ("mrs %0, fpcr" : "=r"(fpcr));
	__asm__ __volatile__ ("msr fpcr, %0" :: "r"(fpcr | 0x1000000)); // enable FZ
#elif defined(__i386__) || defined(__x86_64__)
	const unsigned int flush_zero_mode = _MM_GET_FLUSH_ZERO_MODE();
	const unsigned int denormals_zero_mode = _MM_GET_DENORMALS_ZERO_MODE();

	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif

#if DATA_PRODUCT_CONTROL_INPUTS_N > 0
# ifdef DATA_STATE_DSP_CUSTOM
	_Bool locked = !atomic_flag_test_and_set(&i->sync_lock_flag);
	if (locked) {
		if (!i->synced) {
			if (i->loaded) {
				for (uint32_t j = 0; j < DATA_PRODUCT_CONTROL_INPUTS_N; j++) {
					i->params[j] = i->params_sync[j];
					plugin_set_parameter(&i->p, param_data[j].index, i->params[j]);
				}
			} else {
				for (uint32_t j = 0; j < DATA_PRODUCT_CONTROL_INPUTS_N; j++)
					if (i->params[j] != i->params_sync[j]) {
						i->params_sync[j] = i->params[j];
						plugin_set_parameter(&i->p, param_data[j].index, i->params[j]);
					}
			}
		}
		i->synced = 1;
		i->loaded = 0;
	}
# endif

	for (uint32_t j = 0; j < DATA_PRODUCT_CONTROL_INPUTS_N; j++) {
		if (i->c[j] == NULL)
			continue;
		float v = adjust_param(j, *i->c[j]);
		if (v != i->params[j]) {
			i->params[j] = v;
# ifdef DATA_STATE_DSP_CUSTOM
			if (locked) {
				i->params_sync[j] = i->params[j];
# endif
				plugin_set_parameter(&i->p, param_data[j].index, v);
# ifdef DATA_STATE_DSP_CUSTOM
			}
# endif
		}
	}

# ifdef DATA_STATE_DSP_CUSTOM
	if (locked)
		atomic_flag_clear(&i->sync_lock_flag);
# endif
#endif

#if DATA_PRODUCT_MIDI_INPUTS_N > 0
	// from https://lv2plug.in/book
	if (i->map)
		for (size_t j = 0; j < DATA_PRODUCT_MIDI_INPUTS_N; j++) {
			if (i->x_midi[j] == NULL)
				continue;
			LV2_ATOM_SEQUENCE_FOREACH(i->x_midi[j], ev) {
				if (ev->body.type == i->uri_midi_MidiEvent) {
					const uint8_t * data = (const uint8_t *)(ev + 1);
					if ((data[0] & 0xf0) != 0xf0)
						plugin_midi_msg_in(&i->p, midi_in_index[j], data);
				}
			}
		}
#endif

#if DATA_PRODUCT_AUDIO_INPUT_CHANNELS_N > 0
	const float ** x = i->x;
#else
	const float ** x = NULL;
#endif
#if DATA_PRODUCT_AUDIO_OUTPUT_CHANNELS_N > 0
	float ** y = i-> y;
#else
	float ** y = NULL;
#endif
	plugin_process(&i->p, x, y, sample_count);

#if DATA_PRODUCT_CONTROL_OUTPUTS_N > 0
	for (uint32_t j = 0; j < DATA_PRODUCT_CONTROL_OUTPUTS_N; j++) {
		uint32_t k = param_out_index[j];
		if (i->c[k] != NULL)
			*i->c[k] = plugin_get_parameter(&i->p, k);
	}
#else
	(void)plugin_get_parameter;
#endif

#if defined(__aarch64__)
	__asm__ __volatile__ ("msr fpcr, %0" : : "r"(fpcr));
#elif defined(__i386__) || defined(__x86_64__)
	_MM_SET_FLUSH_ZERO_MODE(flush_zero_mode);
	_MM_SET_DENORMALS_ZERO_MODE(denormals_zero_mode);
#endif
}

static void cleanup(LV2_Handle instance) {
	plugin_instance * i = (plugin_instance *)instance;
	plugin_fini(&i->p);
	if (i->mem)
		free(i->mem);
	free(i->bundle_path);
	free(instance);
}

#ifdef DATA_STATE_DSP_CUSTOM
static LV2_State_Status state_save(LV2_Handle instance, LV2_State_Store_Function store, LV2_State_Handle handle, uint32_t flags, const LV2_Feature * const * features) {
	(void)flags;
	(void)features;

	plugin_instance * i = (plugin_instance *)instance;
	if (!i->map) {
		lv2_log_error(&i->logger, "Host is trying to store state but didn't provide URID map\n");
		return LV2_STATE_ERR_UNKNOWN; // evidently buggy host, we don't have an errcode for it, LV2_STATE_ERR_NO_FEATURE has a different meaning
	}
	i->state_store = store;
	i->state_handle = handle;
	plugin_state_callbacks cbs = {
		/* .handle		= */ (void *)i,
		/* .lock		= */ state_lock_cb,
		/* .unlock		= */ state_unlock_cb,
		/* .write		= */ state_write_cb,
# if DATA_PRODUCT_CONTROL_INPUTS_N > 0
		/* .set_parameter	= */ NULL
# endif
	};
	return plugin_state_save(&i->p, &cbs, i->sample_rate) == 0 ? LV2_STATE_SUCCESS : LV2_STATE_ERR_UNKNOWN;
}

static LV2_State_Status state_restore(LV2_Handle instance, LV2_State_Retrieve_Function retrieve, LV2_State_Handle handle, uint32_t flags, const LV2_Feature * const * features) {
	(void)flags;
	(void)features;

	plugin_instance * i = (plugin_instance *)instance;
	if (!i->map) {
		lv2_log_error(&i->logger, "Host is trying to restore state but didn't provide URID map\n");
		return LV2_STATE_ERR_UNKNOWN; // evidently buggy host, we don't have an errcode for it, LV2_STATE_ERR_NO_FEATURE has a different meaning
	}
	size_t length;
	uint32_t type, xflags; // jalv 1.6.6 crashes using NULL as per spec, so we have these two
	const char * data = retrieve(handle, i->uri_state_data, &length, &type, &xflags);
	if (data == NULL) {
		lv2_log_error(&i->logger, "Cannot restore state since property <%s> could not be retrieved\n", DATA_LV2_URI "#state_data");
		return LV2_STATE_ERR_NO_PROPERTY;
	}
	plugin_state_callbacks cbs = {
		/* .handle		= */ (void *)i,
		/* .lock		= */ state_lock_cb,
		/* .unlock		= */ state_unlock_cb,
		/* .write		= */ NULL,
# if DATA_PRODUCT_CONTROL_INPUTS_N > 0
		/* .set_parameter	= */ state_set_parameter_cb
# endif
	};
	return plugin_state_load(&cbs, i->sample_rate, data, length) == 0 ? LV2_STATE_SUCCESS : LV2_STATE_ERR_UNKNOWN;
}

static const void * extension_data(const char * uri) {
	static const LV2_State_Interface state = { state_save, state_restore };
	if (!strcmp(uri, LV2_STATE__interface))
		return &state;
	return NULL;
}
#endif

static const LV2_Descriptor descriptor = {
	/* .URI			= */ DATA_LV2_URI,
	/* .instantiate		= */ instantiate,
	/* .connect_port	= */ connect_port,
	/* .activate		= */ activate,
	/* .run			= */ run,
	/* .deactivate		= */ NULL,
	/* .cleanup		= */ cleanup,
#ifdef DATA_STATE_DSP_CUSTOM
	/* .extension_data	= */ extension_data
#else
	/* .extension_data	= */ NULL
#endif
};

LV2_SYMBOL_EXPORT const LV2_Descriptor * lv2_descriptor(uint32_t index) {
	return index == 0 ? &descriptor : NULL;
}

#ifdef DATA_UI
typedef struct {
	plugin_ui *		ui;
	char *			bundle_path;
# if DATA_PRODUCT_CONTROL_INPUTS_N > 0
	LV2UI_Write_Function	write;
	LV2UI_Controller	controller;
	char			has_touch;
	LV2UI_Touch		touch;
# endif
# ifdef DATA_MESSAGING
	LV2_URID_Map*      map;
	LV2_Atom_Forge     forge;
	communication_URIs c_uris;
# endif
} ui_instance;

static const char * ui_get_bundle_path_cb(void *handle) {
	ui_instance *instance = (ui_instance *)handle;
	return instance->bundle_path;
}

# if DATA_PRODUCT_CONTROL_INPUTS_N > 0
static void ui_set_parameter_begin_cb(void *handle, size_t index, float value) {
	ui_instance *instance = (ui_instance *)handle;
	if (instance->has_touch) {
		index = index_to_param[index];
		instance->touch.touch(instance->touch.handle, index, true);
		value = adjust_param(index - CONTROL_INPUT_INDEX_OFFSET, value);
		instance->write(instance->controller, index, sizeof(float), 0, &value);
	}
}

static void ui_set_parameter_cb(void *handle, size_t index, float value) {
	ui_instance *instance = (ui_instance *)handle;
	index = index_to_param[index];
	value = adjust_param(index - CONTROL_INPUT_INDEX_OFFSET, value);
	instance->write(instance->controller, index, sizeof(float), 0, &value);
}

static void ui_set_parameter_end_cb(void *handle, size_t index, float value) {
	ui_instance *instance = (ui_instance *)handle;
	if (instance->has_touch) {
		index = index_to_param[index];
		value = adjust_param(index - CONTROL_INPUT_INDEX_OFFSET, value);
		instance->write(instance->controller, index, sizeof(float), 0, &value);
		instance->touch.touch(instance->touch.handle, index, false);
	}
}
# endif

# ifdef DATA_MESSAGING
static char send_to_dsp (void *handle, const void *data, size_t bytes) {
	ui_instance* instance = (ui_instance*) handle;

	printf("send_to_dsp A \n"); fflush(stdout);

	if (!data || bytes <= 0 || bytes > DATA_MESSAGING_MAX) {
		return 1;
	}

	uint8_t obj_buf[256];
	lv2_atom_forge_set_buffer(&instance->forge, obj_buf, sizeof(obj_buf));
	LV2_Atom_Forge_Frame frame;

	// Start forging an object
	LV2_Atom* msg = (LV2_Atom*)lv2_atom_forge_object(&instance->forge, &frame, 0, instance->c_uris.ui_On);
	assert(msg);

	// Add a string property to the object
	lv2_atom_forge_key(&instance->forge, instance->c_uris.prop_customMessage);
	lv2_atom_forge_string(&instance->forge, data, bytes);

	// Finish the object
	lv2_atom_forge_pop(&instance->forge, &frame);

	// Send the forged atom to the host
	instance->write(instance->controller, 8, lv2_atom_total_size(msg), instance->c_uris.atom_eventTransfer, msg);

	return 0;
}
# endif

static LV2UI_Handle ui_instantiate(const LV2UI_Descriptor * descriptor, const char * plugin_uri, const char * bundle_path, LV2UI_Write_Function write_function, LV2UI_Controller controller, LV2UI_Widget * widget, const LV2_Feature * const * features) {
	(void)descriptor;
	(void)plugin_uri;

	ui_instance *instance = malloc(sizeof(ui_instance));
	if (instance == NULL)
		goto err_instance;

	instance->bundle_path = strdup(bundle_path);
	if (instance->bundle_path == NULL)
		goto err_bundle_path;

	char has_parent = 0;
	void *parent = NULL;
# if DATA_PRODUCT_CONTROL_INPUTS_N > 0
	instance->has_touch = 0;
# endif
	for (size_t i = 0; features[i] != NULL; i++) {
		if (!strcmp(features[i]->URI, LV2_UI__parent)) {
			has_parent = 1;
			parent = features[i]->data;
		}
# if DATA_PRODUCT_CONTROL_INPUTS_N > 0
		else if (!strcmp(features[i]->URI, LV2_UI__touch)) {
			instance->has_touch = 1;
			instance->touch = *((LV2UI_Touch *)features[i]->data);
		}
# endif
	}

	plugin_ui_callbacks cbs = {
		/* .handle		= */ (void *)instance,
		/* .format		= */ "lv2",
		/* .get_bindir		= */ ui_get_bundle_path_cb,
		/* .get_datadir		= */ ui_get_bundle_path_cb,
# if DATA_PRODUCT_CONTROL_INPUTS_N > 0
		/* .set_parameter_begin	= */ ui_set_parameter_begin_cb,
		/* .set_parameter	= */ ui_set_parameter_cb,
		/* .set_parameter_end	= */ ui_set_parameter_end_cb,
# endif
# ifdef DATA_MESSAGING
		/* .send_to_dsp = */ send_to_dsp
# endif
	};
# if DATA_PRODUCT_CONTROL_INPUTS_N > 0
	instance->write = write_function;
	instance->controller = controller;
# else
	(void)write_function;
	(void)controller;
# endif
	instance->ui = plugin_ui_create(has_parent, parent, &cbs);
	if (instance->ui == NULL)
		goto err_create;

	*widget = instance->ui->widget;

	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID_URI "#map")) {
			instance->map = (LV2_URID_Map*)features[i]->data;
		}
	}
	lv2_atom_forge_init(&instance->forge, instance->map);
	map_comm_uris(instance->map, &instance->c_uris);

	return instance;

err_create:
	free(instance->bundle_path);
err_bundle_path:
	free(instance);
err_instance:
	*widget = NULL;
	return NULL;
}

static void ui_cleanup(LV2UI_Handle handle) {
	ui_instance *instance = (ui_instance *)handle;
	plugin_ui_free(instance->ui);
	free(instance->bundle_path);
	free(instance);
}

# if DATA_PRODUCT_CONTROL_INPUTS_N + DATA_PRODUCT_CONTROL_OUTPUTS_N > 0
static void ui_port_event(LV2UI_Handle handle, uint32_t port_index, uint32_t buffer_size, uint32_t format, const void * buffer) {
	(void)buffer_size;

	ui_instance *instance = (ui_instance *)handle;
#  if DATA_PRODUCT_CONTROL_INPUTS_N > 0
	if (port_index < CONTROL_OUTPUT_INDEX_OFFSET) {
		size_t index = port_index - CONTROL_INPUT_INDEX_OFFSET;
		plugin_ui_set_parameter(instance->ui, param_data[index].index, adjust_param(index, *((float *)buffer)));
	}
#  endif
#  if DATA_PRODUCT_CONTROL_OUTPUTS_N > 0
#   if DATA_PRODUCT_CONTROL_INPUTS_N > 0
	else
#   endif
		plugin_ui_set_parameter(instance->ui, param_out_index[port_index - CONTROL_OUTPUT_INDEX_OFFSET], *((float *)buffer));
#  endif

	const LV2_Atom* atom = (const LV2_Atom*)buffer;

	/* Check type of data received
	*  - format == 0: Control port event (float)
	*  - format > 0:  Message (atom)
	*/
	if (format == instance->c_uris.atom_eventTransfer && lv2_atom_forge_is_object_type(&instance->forge, atom->type)) {
		const LV2_Atom_Object* obj = (const LV2_Atom_Object*)atom;
		if (obj->body.otype == instance->c_uris.prop_customMessage) {
			printf("received something from audio INCREDIBLE \n");
			const LV2_Atom* len_a = NULL;
			const LV2_Atom* data_a = NULL;
			const int n_props = lv2_atom_object_get(obj, instance->c_uris.length, &len_a, instance->c_uris.prop_customMessage, &data_a, NULL);
			const int32_t len = ((const LV2_Atom_Int*) len_a)->body;
			printf("and n_props is %d \n", n_props);
			printf("and len is %d \n", len);
			printf("and data_a is %p \n", (void*)data_a);
			//printf("and data_a type: %u, size: %u \n", data_a->type, data_a->size);
			const uint8_t* data = (uint8_t*) data_a;
			printf("data... \n");
			for (int x = 0; x < len; x++) {
				printf("%u ", data[x]);
			}
			printf("\n");
		}
	}


}
# endif

static int ui_idle(LV2UI_Handle handle) {
	ui_instance *instance = (ui_instance *)handle;
	plugin_ui_idle(instance->ui);
	return 0;
}

static const void * ui_extension_data(const char * uri) {
	static const LV2UI_Idle_Interface idle = { ui_idle };
	if (!strcmp(uri, LV2_UI__idleInterface))
		return &idle;
	return NULL;
}

static const LV2UI_Descriptor ui_descriptor = {
	/* .URI			= */ DATA_LV2_UI_URI,
	/* .instantiate		= */ ui_instantiate,
	/* .cleanup		= */ ui_cleanup,
# if DATA_PRODUCT_CONTROL_INPUTS_N + DATA_PRODUCT_CONTROL_OUTPUTS_N > 0
	/* .port_event		= */ ui_port_event,
# else
	/* .port_event		= */ NULL,
# endif
	/* .extension_data	= */ ui_extension_data
};

LV2_SYMBOL_EXPORT const LV2UI_Descriptor * lv2ui_descriptor(uint32_t index) {
	return index == 0 ? &ui_descriptor : NULL;
}
#endif
