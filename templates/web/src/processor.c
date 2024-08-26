/*
 * Tibia
 *
 * Copyright (C) 2022-2024 Orastron Srl unipersonale
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

#include <stddef.h>
#include <stdint.h>

#include "data.h"
#include "plugin_api.h"
#include "plugin.h"

#include "string.h"
#include "walloc.h"

typedef struct {
	plugin		p;
	void *		mem;
#if DATA_PRODUCT_AUDIO_INPUT_CHANNELS_N > 0
	float		x_buf[DATA_PRODUCT_AUDIO_INPUT_CHANNELS_N * 128];
	const float *	x[DATA_PRODUCT_AUDIO_INPUT_CHANNELS_N];
	float		zero_buf[128];
#endif
#if DATA_PRODUCT_AUDIO_OUTPUT_CHANNELS_N > 0
	float		y_buf[DATA_PRODUCT_AUDIO_OUTPUT_CHANNELS_N * 128];
	float *		y[DATA_PRODUCT_AUDIO_OUTPUT_CHANNELS_N];
#endif
#if DATA_PRODUCT_PARAMETERS_OUTPUT_N > 0
	float		out_params[DATA_PRODUCT_PARAMETERS_OUTPUT_N];
#endif
} instance;

instance * processor_new(float sample_rate) {
	instance * i = malloc(sizeof(instance));
	if (i == NULL)
		return NULL;

	plugin_callbacks cbs = {
		/* .handle		= */ NULL,
		/* .format		= */ "web",
		/* .get_bindir		= */ NULL,
		/* .get_datadir		= */ NULL
	};
	plugin_init(&i->p, &cbs);

#if DATA_PRODUCT_PARAMETERS_N > 0
	for (size_t j = 0; j < DATA_PRODUCT_PARAMETERS_N; j++)
		if (!param_data[j].out)
			plugin_set_parameter(&i->p, j, param_data[j].def);
#endif

	plugin_set_sample_rate(&i->p, sample_rate);
	size_t req = plugin_mem_req(&i->p);
	if (req != 0) {
		i->mem = malloc(req);
		if (i->mem == NULL) {
			plugin_fini(&i->p);
			return NULL;
		}
		plugin_mem_set(&i->p, i->mem);
	} else
		i->mem = NULL;

	plugin_reset(&i->p);

#if DATA_PRODUCT_AUDIO_INPUT_CHANNELS_N > 0
	memset(i->zero_buf, 0, 128 * sizeof(float));
#endif
#if DATA_PRODUCT_AUDIO_OUTPUT_CHANNELS_N > 0
	for (size_t j = 0; j < DATA_PRODUCT_AUDIO_OUTPUT_CHANNELS_N; j++)
		i->y[j] = i->y_buf + 128 * j;
#endif

	return i;
}

void processor_free(instance * i) {
	plugin_fini(&i->p);
	if (i->mem)
		free(i->mem);
	free(i);
}

float * processor_get_x_buf(instance * i) {
#if DATA_PRODUCT_AUDIO_INPUT_CHANNELS_N > 0
	return i->x_buf;
#else
	(void)i;
	return NULL;
#endif
}

const float ** processor_get_x(instance * i) {
#if DATA_PRODUCT_AUDIO_INPUT_CHANNELS_N > 0
	return i->x;
#else
	(void)i;
	return NULL;
#endif
}

float * processor_get_zero_buf(instance * i) {
#if DATA_PRODUCT_AUDIO_INPUT_CHANNELS_N > 0
	return i->zero_buf;
#else
	(void)i;
	return NULL;
#endif
}

float * processor_get_y_buf(instance * i) {
#if DATA_PRODUCT_AUDIO_OUTPUT_CHANNELS_N > 0
	return i->y_buf;
#else
	(void)i;
	return NULL;
#endif
}

float * processor_get_out_params(instance *i) {
#if DATA_PRODUCT_PARAMETERS_OUTPUT_N > 0
	return i->out_params;
#else
	(void)i;
	return NULL;
#endif
}

void processor_set_parameter(instance *i, int32_t index, float value) {
	plugin_set_parameter(&i->p, index, value);
}

void processor_process(instance *i, int32_t n_samples) {
#if DATA_PRODUCT_AUDIO_INPUT_CHANNELS_N > 0
	const float **x = i->x;
#else
	const float **x = NULL;
#endif
#if DATA_PRODUCT_AUDIO_OUTPUT_CHANNELS_N > 0
	float **y = i->y;
#else
	float **y = NULL;
#endif

	plugin_process(&i->p, x, y, n_samples);

#if DATA_PRODUCT_PARAMETERS_OUTPUT_N > 0
	for (size_t j = 0; j < DATA_PRODUCT_PARAMETERS_OUTPUT_N; j++)
		i->out_params[j] = plugin_get_parameter(&i->p, param_out_index[j]);
#else
	(void)plugin_get_parameter;
#endif
}

#if DATA_PRODUCT_MIDI_INPUTS_N > 0
void processor_midi_msg_in(instance *i, int32_t index, uint8_t data0, uint8_t data1, uint8_t data2) {
	uint8_t data[3] = { data0, data1, data2 };
	plugin_midi_msg_in(&i->p, index, data);
}
#endif
