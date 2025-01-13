/*
 * Tibia
 *
 * Copyright (C) 2023-2025 Orastron Srl unipersonale
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

#include <stdint.h>

typedef struct plugin {
	plugin_callbacks	cbs;

	float			sample_rate;
	size_t			delay_line_length;

	float			gain;
	float			delay;
	float			cutoff;
	char			bypass;

	float *			delay_line;
	size_t			delay_line_cur;
	float			z1;
	float			cutoff_k;
	float			yz1;
} plugin;

static void plugin_init(plugin *instance, plugin_callbacks *cbs) {
	instance->cbs = *cbs;
}

static void plugin_fini(plugin *instance) {
	(void)instance;
}

static void plugin_set_sample_rate(plugin *instance, float sample_rate) {
	instance->sample_rate = sample_rate;
	//safe approx instance->delay_line_length = ceilf(sample_rate) + 1;
	instance->delay_line_length = (size_t)(sample_rate + 1.f) + 1;
}

static size_t plugin_mem_req(plugin *instance) {
	return instance->delay_line_length * sizeof(float);
}

static void plugin_mem_set(plugin *instance, void *mem) {
	instance->delay_line = (float *)mem;
}

static void plugin_reset(plugin *instance) {
	for (size_t i = 0; i < instance->delay_line_length; i++)
		instance->delay_line[i] = 0.f;
	instance->delay_line_cur = 0;
	instance->z1 = 0.f;
	instance->cutoff_k = 1.f;
	instance->yz1 = 0.f;
}

static void plugin_set_parameter(plugin *instance, size_t index, float value) {
	switch (index) {
	case plugin_parameter_gain:
		instance->gain = value;
		break;
	case plugin_parameter_delay:
		instance->delay = value;
		break;
	case plugin_parameter_cutoff:
		instance->cutoff = value;
		break;
	case plugin_parameter_bypass:
		instance->bypass = value >= 0.5f;
		break;
	}
}

static float plugin_get_parameter(plugin *instance, size_t index) {
	(void)index;
	return instance->yz1;
}

static size_t calc_index(size_t cur, size_t delay, size_t len) {
	return (cur < delay ? cur + len : cur) - delay;
}

static void plugin_process(plugin *instance, const float **inputs, float **outputs, size_t n_samples) {
	//approx const float gain = powf(10.f, 0.05f * instance->gain);
	const float gain = ((2.6039890429412597e-4f * instance->gain + 0.032131027163547855f) * instance->gain + 1.f) / ((0.0012705124328080768f * instance->gain - 0.0666763481312185f) * instance->gain + 1.f);
	//approx const size_t delay = roundf(instance->sample_rate * 0.001f * instance->delay);
	const size_t delay = (size_t)(instance->sample_rate * 0.001f * instance->delay + 0.5f);
	const float mA1 = instance->sample_rate / (instance->sample_rate + 6.283185307179586f * instance->cutoff * instance->cutoff_k);
	for (size_t i = 0; i < n_samples; i++) {
		instance->delay_line[instance->delay_line_cur] = inputs[0][i];
		const float x = instance->delay_line[calc_index(instance->delay_line_cur, delay, instance->delay_line_length)];
		instance->delay_line_cur++;
		if (instance->delay_line_cur == instance->delay_line_length)
			instance->delay_line_cur = 0;
		const float y = x + mA1 * (instance->z1 - x);
		instance->z1 = y;
		outputs[0][i] = instance->bypass ? inputs[0][i] : gain * y;
		instance->yz1 = outputs[0][i];
	}
}

static void plugin_midi_msg_in(plugin *instance, size_t index, const uint8_t * data) {
	(void)index;
	if (((data[0] & 0xf0) == 0x90) && (data[2] != 0))
		//approx instance->cutoff_k = powf(2.f, (1.f / 12.f) * (note - 60));
		instance->cutoff_k = data[1] < 64 ? (-0.19558034980097166f * data[1] - 2.361735109225749f) / (data[1] - 75.57552349522389f) : (393.95397927344214f - 7.660826245588588f * data[1]) / (data[1] - 139.0755234952239f);
}

static void serialize_float(uint8_t *dest, float f) {
	union { float f; uint32_t u; } v;
	v.f = f;
	dest[0] = v.u & 0xff;
	dest[1] = (v.u & 0xff00) >> 8;
	dest[2] = (v.u & 0xff0000) >> 16;
	dest[3] = (v.u & 0xff000000) >> 24;
}

static float parse_float(const uint8_t *data) {
	union { float f; uint32_t u; } v;
	v.u = data[0];
	v.u |= data[1] << 8;
	v.u |= data[2] << 16;
	v.u |= data[3] << 24;
	return v.f;
}

static int plugin_state_save(plugin *instance) {
	uint8_t data[13];
	serialize_float(data, instance->gain);
	serialize_float(data + 4, instance->delay);
	serialize_float(data + 8, instance->cutoff);
	data[12] = instance->bypass ? 1 : 0;
	return instance->cbs.write_state(instance->cbs.handle, (const char *)data, 13);
}

static void plugin_state_load(plugin *instance, const char *data, size_t length) {
	(void)length;
	const uint8_t *d = (const uint8_t *)data;
	instance->cbs.load_parameter(instance->cbs.handle, plugin_parameter_gain, parse_float(d));
	instance->cbs.load_parameter(instance->cbs.handle, plugin_parameter_delay, parse_float(d + 4));
	instance->cbs.load_parameter(instance->cbs.handle, plugin_parameter_cutoff, parse_float(d + 8));
	instance->cbs.load_parameter(instance->cbs.handle, plugin_parameter_bypass, d[12] ? 1.f : 0.f);
}
