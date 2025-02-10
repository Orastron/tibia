/*
 * Tibia
 *
 * Copyright (C) 2024 Orastron Srl unipersonale
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

#include "lv2/core/lv2.h"
#include "lv2/core/lv2_util.h"
#include "lv2/log/log.h"
#include "lv2/log/logger.h"
#include "lv2/urid/urid.h"
#if DATA_PRODUCT_MIDI_INPUTS_N + DATA_PRODUCT_MIDI_OUTPUTS_N > 0
# include "lv2/atom/util.h"
# include "lv2/atom/atom.h"
# include "lv2/midi/midi.h"
#endif
#ifdef DATA_UI
# include "lv2/ui/ui.h"
#endif

#include <string.h>

#if defined(__i386__) || defined(__x86_64__)
# include <xmmintrin.h>
# include <pmmintrin.h>
#endif

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

typedef struct {
	plugin				p;
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
#endif
	void *				mem;
	char *				bundle_path;
	LV2_Log_Logger			logger;
	LV2_URID_Map *			map;
#if DATA_PRODUCT_MIDI_INPUTS_N + DATA_PRODUCT_MIDI_OUTPUTS_N > 0
	LV2_URID			uri_midi_MidiEvent;
#endif
} plugin_instance;

static const char * get_bundle_path_cb(void *handle) {
	plugin_instance *instance = (plugin_instance *)handle;
	return instance->bundle_path;
}

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
	const char* missing = lv2_features_query(features,
		LV2_LOG__log,	&instance->logger.log,	false,
		LV2_URID__map,	&instance->map,		true,
		NULL);

	lv2_log_logger_set_map(&instance->logger, instance->map);
	if (missing) {
		lv2_log_error(&instance->logger, "Missing feature <%s>\n", missing);
		goto err_urid;
	}

#if DATA_PRODUCT_MIDI_INPUTS_N + DATA_PRODUCT_MIDI_OUTPUTS_N > 0
	instance->uri_midi_MidiEvent = instance->map->map(instance->map->handle, LV2_MIDI__MidiEvent);
#endif

	plugin_callbacks cbs = {
		/* .handle		= */ (void *)instance,
		/* .format		= */ "lv2",
		/* .get_bindir		= */ get_bundle_path_cb,
		/* .get_datadir		= */ get_bundle_path_cb
	};
	plugin_init(&instance->p, &cbs);

	plugin_set_sample_rate(&instance->p, sample_rate);
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

	return instance;

err_mem:
	plugin_fini(&instance->p);
err_urid:
	free(instance->bundle_path);
err_bundle_path:
	free(instance);
err_instance:
	return NULL;
}

static void connect_port(LV2_Handle instance, uint32_t port, void * data_location) {
	plugin_instance * i = (plugin_instance *)instance;
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
#endif
	plugin_reset(&i->p);
}

static void run(LV2_Handle instance, uint32_t sample_count) {
	plugin_instance * i = (plugin_instance *)instance;

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
	for (uint32_t j = 0; j < DATA_PRODUCT_CONTROL_INPUTS_N; j++) {
		if (i->c[j] == NULL)
			continue;
		float v = adjust_param(j, *i->c[j]);
		if (v != i->params[j]) {
			i->params[j] = v;
			plugin_set_parameter(&i->p, param_data[j].index, v);
		}
	}
#endif

	// from https://lv2plug.in/book
#if DATA_PRODUCT_MIDI_INPUTS_N > 0
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

static const LV2_Descriptor descriptor = {
	/* .URI			= */ DATA_LV2_URI,
	/* .instantiate		= */ instantiate,
	/* .connect_port	= */ connect_port,
	/* .activate		= */ activate,
	/* .run			= */ run,
	/* .deactivate		= */ NULL,
	/* .cleanup		= */ cleanup,
	/* .extension_data	= */ NULL
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
} ui_instance;

# define CONTROL_INPUT_INDEX_OFFSET ( \
		DATA_PRODUCT_AUDIO_INPUT_CHANNELS_N \
		+ DATA_PRODUCT_AUDIO_OUTPUT_CHANNELS_N \
		+ DATA_PRODUCT_MIDI_INPUTS_N \
		+ DATA_PRODUCT_MIDI_OUTPUTS_N )
# define CONTROL_OUTPUT_INDEX_OFFSET	(CONTROL_INPUT_INDEX_OFFSET + DATA_PRODUCT_CONTROL_INPUTS_N)

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
	instance->has_touch = 0;
	for (size_t i = 0; features[i] != NULL; i++) {
		if (!strcmp(features[i]->URI, LV2_UI__parent)) {
			has_parent = 1;
			parent = features[i]->data;
		}
		if (!strcmp(features[i]->URI, LV2_UI__touch)) {
			instance->has_touch = 1;
			instance->touch = *((LV2UI_Touch *)features[i]->data);
		}
	}

	plugin_ui_callbacks cbs = {
		/* .handle		= */ (void *)instance,
		/* .format		= */ "lv2",
		/* .get_bindir		= */ ui_get_bundle_path_cb,
		/* .get_datadir		= */ ui_get_bundle_path_cb,
# if DATA_PRODUCT_CONTROL_INPUTS_N > 0
		/* .set_parameter_begin	= */ ui_set_parameter_begin_cb,
		/* .set_parameter	= */ ui_set_parameter_cb,
		/* .set_parameter_end	= */ ui_set_parameter_end_cb
# else
		/* .set_parameter_begin	= */ NULL,
		/* .set_parameter	= */ NULL,
		/* .set_parameter_end	= */ NULL
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
	(void)format;

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
