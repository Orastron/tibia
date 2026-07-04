/*
 * Tibia
 *
 * Copyright (C) 2024-2026 Orastron Srl unipersonale
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

#ifndef PLUGIN_API_H
#define PLUGIN_API_H

typedef struct {
	void *		handle;
	const char *	format;
	const char * (*get_bindir)(void *handle);
	const char * (*get_datadir)(void *handle);
} plugin_callbacks;

{{?it.product.state && it.product.state.dspCustom}}
typedef struct {
	void *	handle;
	void (*lock)(void *handle);
	void (*unlock)(void *handle);
	int (*write)(void *handle, const char *data, size_t length);
{{?it.product.parameters.find(x => x.direction == "input")}}
	void (*set_parameter)(void *handle, size_t index, float value);
{{?}}
} plugin_state_callbacks;
{{?}}

typedef struct {
	void *		handle;
	const char *	format;
	const char * (*get_bindir)(void *handle);
	const char * (*get_datadir)(void *handle);
{{?it.product.parameters.find(x => x.direction == "input")}}
	void (*set_parameter_begin)(void *handle, size_t index, float value);
	void (*set_parameter)(void *handle, size_t index, float value);
	void (*set_parameter_end)(void *handle, size_t index, float value);
{{?}}
} plugin_ui_callbacks;

{{?it.product.parameters.length > 0}}
enum {
	{{~it.product.parameters :p}}
	plugin_parameter_{{=p.id}},
	{{~}}
	plugin_parameter__count
};
{{?}}

{{?it.product.state}}
#define PLUGIN_HAS_STATE	1
{{?}}

{{?(it.product.transport && it.product.transport.sync)}}
#include <stdint.h>

#define PLUGIN_TRANSPORT_PLAYING	((uint32_t)1)
#define PLUGIN_TRANSPORT_SPEED		((uint32_t)(1 << 1))
#define PLUGIN_TRANSPORT_BPM		((uint32_t)(1 << 2))
#define PLUGIN_TRANSPORT_QUARTER	((uint32_t)(1 << 3))
#define PLUGIN_TRANSPORT_BEAT		((uint32_t)(1 << 4))
#define PLUGIN_TRANSPORT_TIME_SIG_NUM	((uint32_t)(1 << 5))
#define PLUGIN_TRANSPORT_TIME_SIG_DENOM	((uint32_t)(1 << 6))

typedef struct {
	uint32_t	changed;
	uint32_t	valid;
	char		playing;
	float		speed;
	float		bpm;
	double		quarter;
	double		beat;
	uint32_t	time_sig_num;
	uint32_t	time_sig_denom;
} plugin_transport;
{{?}}

#endif
