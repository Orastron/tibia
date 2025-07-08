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
 * File author: Stefano D'Angelo, Paolo Marrone
 */

#include "vinci.h"
#include <stdio.h>
#include <string.h>

typedef struct {
	void   *widget;

	vinci  *vinci;
	window *window;
	int     param_down;

	float gain;
	float delay;
	float cutoff;
	char  bypass;
	float y_z1;

	plugin_ui_callbacks	cbs;
} plugin_ui;

#ifdef TEMPLATE_SUPPORTS_MESSAGING
static void plugin_ui_receive_from_dsp (plugin_ui *instance, const void *data, size_t bytes) {
	(void) instance;
	printf("plugin_ui_receive_from_dsp %lld bytes at %p: ", bytes, data);
	for (size_t i = 0; i < bytes; i++) {
		printf("%c", ((uint8_t*) data)[i]);
	}
	printf("\nplugin_ui_receive_from_dsp END \n");
}
#define RANDOM_UI_DATA_SIZE 6
const uint8_t random_ui_data[RANDOM_UI_DATA_SIZE] = { 'h', 'e', 'l', 'l', 'o', 0 };
#endif

#define WIDTH		600.0
#define HEIGHT		400.0

static void plugin_ui_get_default_size(uint32_t *width, uint32_t *height) {
	*width = WIDTH;
	*height = HEIGHT;
}

static void draw_rect(window *w, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
	uint32_t *data = (uint32_t*) malloc(width * height * 4);
	uint32_t p = 0;
	for (uint32_t i = 0; i < height; i++)
		for (uint32_t j = 0; j < width; j++, p++)
			data[p] = color;
	window_draw(w, (unsigned char*)data, 0, 0, width, height, x, y, width, height);
	free(data);
}

static void draw_slider(plugin_ui *pui, int id, float value) {
	const int w = window_get_width(pui->window);
	const int h = window_get_height(pui->window);
	draw_rect(pui->window, 0.1 * w, 0.15 * (id + 1) * h, 0.8 * w * value, 0.1 * h, 0x6789ab);
	draw_rect(pui->window, 0.1 * w + 0.8 * w * value, 0.15 * (id + 1) * h, 0.8 * w * (1.f - value), 0.1 * h, 0x1223bc);
}

static void draw_button(plugin_ui *pui, int id, char value) {
	const int w = window_get_width(pui->window);
	const int h = window_get_height(pui->window);
	draw_rect(pui->window, 0.4 * w, 0.15 * (id + 1) * h, 0.2 * w, 0.1 * h, value ? 0x6789ab : 0x1223bc);
}

static void on_close(window *w) {
	printf("on_close %p \n", (void*)w); fflush(stdout);
}

static void on_mouse_press (window *win, int32_t x, int32_t y, uint32_t state) {
	(void) state;

	plugin_ui *pui = (plugin_ui*) window_get_data(win);
	const int w = window_get_width(win);
	const int h = window_get_height(win);

	if (x >= 0.1 * w && x <= 0.9 * w && y >= 0.15 * h && y <= 0.25 * h) {
		pui->param_down = 0;
		pui->gain = (float)((x - (0.1 * w)) / (0.8 * w));
		pui->cbs.set_parameter_begin(pui->cbs.handle, 0, -60.f + 80.f * pui->gain);
		draw_slider(pui, 0, pui->gain);
	} else if (x >= 0.1 * w && x <= 0.9 * w && y >= 0.3 * h && y <= 0.4 * h) {
		pui->param_down = 1;
		pui->delay = (float)((x - (0.1 * w)) / (0.8 * w));
		pui->cbs.set_parameter_begin(pui->cbs.handle, 1, 1000.f * pui->delay);
		draw_slider(pui, 1, pui->delay);
	} else if (x >= 0.1 * w && x <= 0.9 * w && y >= 0.45 * h && y <= 0.55 * h) {
		pui->param_down = 2;
		pui->cutoff = (float)((x - (0.1 * w)) / (0.8 * w));
		pui->cbs.set_parameter_begin(pui->cbs.handle, 2, (632.4555320336746f * pui->cutoff + 20.653108640674372f) / (1.0326554320337158f - pui->cutoff));
		draw_slider(pui, 2, pui->cutoff);
	} else if (x >= 0.4 * w && x <= 0.6 * w && y >= 0.6 * h && y <= 0.7 * h) {
		pui->param_down = 4;
	}
}

static void on_mouse_release (window *win, int32_t x, int32_t y, uint32_t state) {
	(void) state;

	plugin_ui *pui = (plugin_ui*) window_get_data(win);
	const int w = window_get_width(win);
	const int h = window_get_height(win);

	if (pui->param_down == 4)
		if (x >= 0.4 * w && x <= 0.6 * w && y >= 0.6 * h && y <= 0.7 * h) {
			pui->bypass = !pui->bypass;
			pui->cbs.set_parameter(pui->cbs.handle, 3, pui->bypass ? 1.f : 0.f);
			draw_button(pui, 3, pui->bypass);
#if TEMPLATE_SUPPORTS_MESSAGING
			pui->cbs.send_to_dsp(pui->cbs.handle, (const void*) random_ui_data, RANDOM_UI_DATA_SIZE);
#endif
		}

	if (pui->param_down != -1) {
		float v = x < 0.1 * w ? 0.f : (x > 0.9 * w ? 1.f : (float)((x - (0.1 * w)) / (0.8 * w)));
		switch (pui->param_down) {
		case 0:
			pui->gain = v;
			pui->cbs.set_parameter_end(pui->cbs.handle, 0, -60.f + 80.f * pui->gain);
			draw_slider(pui, 0, pui->gain);
			break;
		case 1:
			pui->delay = v;
			pui->cbs.set_parameter_end(pui->cbs.handle, 1, 1000.f * pui->delay);
			draw_slider(pui, 1, pui->delay);
			break;
		case 2:
			pui->cutoff = v;
			pui->cbs.set_parameter_end(pui->cbs.handle, 2, (632.4555320336746f * pui->cutoff + 20.653108640674372f) / (1.0326554320337158f - pui->cutoff));
			draw_slider(pui, 2, pui->cutoff);
			break;
		}
		pui->param_down = -1;
	}
}

static void on_mouse_move (window *win, int32_t x, int32_t y, uint32_t state) {
	(void) y;
	(void) state;

	plugin_ui *pui = (plugin_ui*) window_get_data(win);
	const int w = window_get_width(win);
	
	float v = x < 0.1 * w ? 0.f : (x > 0.9 * w ? 1.f : (float)((x - (0.1 * w)) / (0.8 * w)));

	switch (pui->param_down) {
	case 0:
		pui->gain = v;
		pui->cbs.set_parameter(pui->cbs.handle, 0, -60.f + 80.f * pui->gain);
		draw_slider(pui, 0, pui->gain);
		break;
	case 1:
		pui->delay = v;
		pui->cbs.set_parameter(pui->cbs.handle, 1, 1000.f * pui->delay);
		draw_slider(pui, 1, pui->delay);
		break;
	case 2:
		pui->cutoff = v;
		pui->cbs.set_parameter(pui->cbs.handle, 2, (632.4555320336746f * pui->cutoff + 20.653108640674372f) / (1.0326554320337158f - pui->cutoff));
		draw_slider(pui, 2, pui->cutoff);
		break;
	}
}

static void on_window_resize (window *w, int32_t width, int32_t height) {
	draw_rect(w, 0, 0, width, height, 0xff9999);

	plugin_ui *pui = (plugin_ui*) window_get_data(w);

	draw_slider(pui, 0, pui->gain);
	draw_slider(pui, 1, pui->delay);
	draw_slider(pui, 2, pui->cutoff);
	draw_button(pui, 3, pui->bypass);
	draw_slider(pui, 4, pui->y_z1);
}

static plugin_ui *plugin_ui_create(char has_parent, void *parent, plugin_ui_callbacks *cbs) {
	plugin_ui *instance = (plugin_ui *) malloc(sizeof(plugin_ui));
	if (instance == NULL)
		return NULL;

	struct window_cbs wcbs;
	memset(&wcbs, 0, sizeof(window_cbs));
	wcbs.on_window_close  = on_close;
	wcbs.on_mouse_press   = on_mouse_press;
	wcbs.on_mouse_release = on_mouse_release;
	wcbs.on_mouse_move    = on_mouse_move;
	wcbs.on_window_resize = on_window_resize;

	instance->param_down = -1;
	instance->vinci  = vinci_new();
	instance->window = window_new(instance->vinci, has_parent ? parent : NULL, WIDTH, HEIGHT, &wcbs);
	instance->widget = window_get_handle(instance->window);
	window_set_data(instance->window, (void*) instance);
	window_show(instance->window);

	// just some valid values to allow drawing
	instance->gain = 0.f;
	instance->delay = 0.f;
	instance->cutoff = 0.f;
	instance->bypass = 0;
	instance->y_z1 = 0.f;

	on_window_resize(instance->window, window_get_width(instance->window), window_get_height(instance->window));

	instance->cbs = *cbs;
	return instance;
}

static void plugin_ui_free(plugin_ui *instance) {
	window_free(instance->window);
	vinci_destroy(instance->vinci);
	free(instance);
}

static void plugin_ui_idle(plugin_ui *instance) {
	vinci_idle(instance->vinci);
}

static void plugin_ui_set_parameter(plugin_ui *instance, size_t index, float value) {
	switch (index) {
	case 0:
		instance->gain = 0.0125f * value + 0.75f;
		draw_slider(instance, 0, instance->gain);
		break;
	case 1:
		instance->delay = 0.001f * value;
		draw_slider(instance, 1, instance->delay);
		break;
	case 2:
		// (bad) approx log unmap
		instance->cutoff = (1.0326554320337176f * value - 20.65310864067435f) / (value + 632.4555320336754f);
		draw_slider(instance, 2, instance->cutoff);
		break;
	case 3:
		instance->bypass = value >= 0.5f;
		draw_button(instance, 3, instance->bypass);
		break;
	case 4:
		instance->y_z1 = 0.5f * value + 0.5f;
		draw_slider(instance, 4, instance->y_z1);
		break;
	}
}
