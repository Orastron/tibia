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
{{?(it.product.messaging && it.product.messaging.dspToUiSize)}}
	void (*msg_write)(void *handle, size_t size, const void *data);
{{?}}
#ifdef PLUGIN_CALLBACKS_EXTRA
	PLUGIN_CALLBACKS_EXTRA
#endif
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
#ifdef PLUGIN_STATE_CALLBACKS_EXTRA
	PLUGIN_STATE_CALLBACKS_EXTRA
#endif
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
{{?(it.product.messaging && it.product.messaging.uiToDspSize)}}
	void (*msg_write)(void *handle, size_t size, const void *data);
{{?}}
#ifdef PLUGIN_UI_CALLBACKS_EXTRA
	PLUGIN_UI_CALLBACKS_EXTRA
#endif
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
#define PLUGIN_TRANSPORT_BAR		((uint32_t)(1 << 7))
#define PLUGIN_TRANSPORT_BAR_BEAT	((uint32_t)(1 << 8))

typedef struct {
	uint32_t	changed;		// what changed, flag mask
	uint32_t	valid;			// waht is valid, flag mask
	char		playing;		// 0 = stopped, non-0 = playing
	float		speed;			// playback speed ratio
	float		bpm;			// beats per minute
	double		quarter;		// quarter notes since "origin"
	double		beat;			// beats since "origin"
	float		time_sig_num;		// time signature numerator
	uint32_t	time_sig_denom;		// time signature denominator
	uint64_t	bar;			// bars since "origin"
	float		bar_beat;		// beats since beginning of the current bar
} plugin_transport;
{{?}}

#endif
