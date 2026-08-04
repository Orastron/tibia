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
 * File author: Stefano D'Angelo, Paolo Marrone
 */

#include "vinci.h"
#include <stdio.h>
#include <string.h>

typedef struct {
	void *			widget;

	vinci *			ui;
	window *		w;
	int     		param_down;

	float			gain;
	float			delay;
	float			cutoff;
	float			tremolo;
	char			bypass;
	float			y_z1;

	float			count;

	plugin_ui_callbacks	cbs;
} plugin_ui;

#define WIDTH		600.0
#define HEIGHT		600.0

#define NUM_PARAMS	6

#define SPACES		(NUM_PARAMS + NUM_PARAMS + 1)

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
	const int w = window_get_width(pui->w);
	const int h = window_get_height(pui->w);
	const double sh = h / SPACES;
	draw_rect(pui->w, 0.1 * w, (2 * id + 1) * sh, 0.8 * w * value, sh, 0x6789ab);
	draw_rect(pui->w, 0.1 * w + 0.8 * w * value, (2 * id + 1) * sh, 0.8 * w * (1.f - value), sh, 0x1223bc);
}

static void draw_button(plugin_ui *pui, int id, char value) {
	const int w = window_get_width(pui->w);
	const int h = window_get_height(pui->w);
	const double sh = h / SPACES;
	draw_rect(pui->w, 0.4 * w, (2 * id + 1) * sh, 0.2 * w, sh, value ? 0x6789ab : 0x1223bc);
}

static void draw_count(plugin_ui *pui) {
	const int w = window_get_width(pui->w);
	const int h = window_get_height(pui->w);
	const double sh = h / SPACES;
	const uint8_t r = (1 * pui->count - (int)(1 * pui->count)) * 0xff;
	const uint8_t g = (0.5f * pui->count - (int)(0.5f * pui->count)) * 0xff;
	const uint8_t b = ((1.f / 3.f) * pui->count - (int)((1.f / 3.f) * pui->count)) * 0xff;
	draw_rect(pui->w, 0.8 * w, (2 * 4 + 1) * sh, 0.1 * w, sh, (r << 16) | (g << 8) | b);
}

static void on_close(window *w) {
	printf("on_close %p \n", (void*)w); fflush(stdout);
}

static void on_mouse_press(window *win, int32_t x, int32_t y, uint32_t state) {
	(void) state;

	plugin_ui *pui = (plugin_ui*) window_get_data(win);
	const int w = window_get_width(win);
	const int h = window_get_height(win);
	const double sh = h / SPACES;

	if (x >= 0.1 * w && x <= 0.9 * w && y >= sh && y <= 2 * sh) {
		pui->param_down = 0;
		pui->gain = (float)((x - (0.1 * w)) / (0.8 * w));
		pui->cbs.set_parameter_begin(pui->cbs.handle, 0, -60.f + 80.f * pui->gain);
		draw_slider(pui, 0, pui->gain);
	} else if (x >= 0.1 * w && x <= 0.9 * w && y >= 3 * sh && y <= 4 * sh) {
		pui->param_down = 1;
		pui->delay = (float)((x - (0.1 * w)) / (0.8 * w));
		pui->cbs.set_parameter_begin(pui->cbs.handle, 1, 1000.f * pui->delay);
		draw_slider(pui, 1, pui->delay);
	} else if (x >= 0.1 * w && x <= 0.9 * w && y >= 5 * sh && y <= 6 * sh) {
		pui->param_down = 2;
		pui->cutoff = (float)((x - (0.1 * w)) / (0.8 * w));
		pui->cbs.set_parameter_begin(pui->cbs.handle, 2, (632.4555320336746f * pui->cutoff + 20.653108640674372f) / (1.0326554320337158f - pui->cutoff));
		draw_slider(pui, 2, pui->cutoff);
	} else if (x >= 0.1 * w && x <= 0.9 * w && y >= 7 * sh && y <= 8 * sh) {
		pui->param_down = 3;
		pui->tremolo = (float)((x - (0.1 * w)) / (0.8 * w));
		pui->cbs.set_parameter_begin(pui->cbs.handle, 3, 100.f * pui->tremolo);
		draw_slider(pui, 3, pui->tremolo);
	} else if (x >= 0.4 * w && x <= 0.6 * w && y >= 9 * sh && y <= 10 * sh) {
		pui->param_down = 4;
	} else if (x >= 0.8 * w && x <= 0.9 * w && y >= 9 * sh && y <= 10 * sh) {
		pui->param_down = 5;
	}
}

static void on_mouse_release(window *win, int32_t x, int32_t y, uint32_t state) {
	(void) state;

	plugin_ui *pui = (plugin_ui*) window_get_data(win);
	const int w = window_get_width(win);
	const int h = window_get_height(win);
	const double sh = h / SPACES;

	if (pui->param_down == 4)
		if (x >= 0.4 * w && x <= 0.6 * w && y >= 9 * sh && y <= 10 * sh) {
			pui->bypass = !pui->bypass;
			pui->cbs.set_parameter(pui->cbs.handle, 4, pui->bypass ? 1.f : 0.f);
			draw_button(pui, 4, pui->bypass);
		}
	if (pui->param_down == 5)
		if (x >= 0.8 * w && x <= 0.9 * w && y >= 9 * sh && y <= 10 * sh)
			pui->cbs.msg_write(pui->cbs.handle, 5, "reset");

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
		case 3:
			pui->tremolo = v;
			pui->cbs.set_parameter_end(pui->cbs.handle, 3, 100.f * pui->tremolo);
			draw_slider(pui, 3, pui->tremolo);
			break;
		}
		pui->param_down = -1;
	}
}

static void on_mouse_move(window *win, int32_t x, int32_t y, uint32_t state) {
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
	case 3:
		pui->tremolo = v;
		pui->cbs.set_parameter(pui->cbs.handle, 3, 100.f * pui->tremolo);
		draw_slider(pui, 3, pui->tremolo);
	}
}

static void on_window_resize(window *w, int32_t width, int32_t height) {
	draw_rect(w, 0, 0, width, height, 0xff9999);

	plugin_ui *pui = (plugin_ui*) window_get_data(w);

	draw_slider(pui, 0, pui->gain);
	draw_slider(pui, 1, pui->delay);
	draw_slider(pui, 2, pui->cutoff);
	draw_slider(pui, 3, pui->tremolo);
	draw_button(pui, 4, pui->bypass);
	draw_slider(pui, 5, pui->y_z1);
	draw_count(pui);
}

static void on_key_press(window *w, uint32_t keycode, uint32_t state) {
	printf("on_key_press %p %u\n", (void*)w, keycode); fflush(stdout);
}

static void on_key_release(window *w, uint32_t keycode, uint32_t state) {
	printf("on_key_release %p %u\n", (void*)w, keycode); fflush(stdout);
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
	wcbs.on_key_press     = on_key_press;
	wcbs.on_key_release   = on_key_release;

	instance->param_down = -1;
	instance->ui  = vinci_new();
	instance->w = window_new(instance->ui, has_parent ? parent : NULL, WIDTH, HEIGHT, 1, &wcbs);
	instance->widget = window_get_handle(instance->w);
	window_set_data(instance->w, (void*) instance);

	// just some valid values to allow drawing
	instance->gain = 0.f;
	instance->delay = 0.f;
	instance->cutoff = 0.f;
	instance->tremolo = 0.f;
	instance->bypass = 0;
	instance->y_z1 = 0.f;

	on_window_resize(instance->w, window_get_width(instance->w), window_get_height(instance->w));

	instance->cbs = *cbs;
	return instance;
}

static void plugin_ui_free(plugin_ui *instance) {
	window_free(instance->w);
	vinci_destroy(instance->ui);
	free(instance);
}

static void plugin_ui_idle(plugin_ui *instance) {
	vinci_idle(instance->ui);
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
		instance->tremolo = 0.01f * value;
		draw_slider(instance, 3, instance->tremolo);
		break;
	case 4:
		instance->bypass = value >= 0.5f;
		draw_button(instance, 4, instance->bypass);
		break;
	case 5:
		instance->y_z1 = 0.5f * value + 0.5f;
		draw_slider(instance, 5, instance->y_z1);
		break;
	}
}

static void plugin_ui_msg_in(plugin_ui *instance, size_t size, const void * data) {
	(void)instance;
	char *s = (char *)alloca(size + 1);
	memcpy(s, data, size);
	s[size] = '\0';
	sscanf(s, "%f", &instance->count);
	printf("%.*s -> %g\n", (int)size, (const char *)data, instance->count);
	draw_count(instance);
}
