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
 * File author: Stefano D'Angelo, Paolo Marrone
 */

#if defined(_WIN32) || defined(__CYGWIN__)
#elif defined(__APPLE__)
# include "ui_apple.h"
#else
# include <X11/Xlib.h>
#endif

typedef struct {
	void *			widget;

#if defined(_WIN32) || defined(__CYGWIN__)
#elif defined(__APPLE__)
#else
	Display *		display;
#endif

	float			gain;
	float			delay;
	float			cutoff;
	char			bypass;
	float			y_z1;

	unsigned int		width;
	unsigned int		height;

	plugin_ui_callbacks	cbs;
} plugin_ui;

#define WIDTH		600.0
#define HEIGHT		400.0

static void plugin_ui_get_default_size(uint32_t *width, uint32_t *height) {
	*width = WIDTH;
	*height = HEIGHT;
}

static plugin_ui *plugin_ui_create(char has_parent, void *parent, plugin_ui_callbacks *cbs) {
	plugin_ui *instance = malloc(sizeof(plugin_ui));
	if (instance == NULL)
		return NULL;
#if defined(_WIN32) || defined(__CYGWIN__)
#elif defined(__APPLE__)
	instance->widget = (void *)ui_create(has_parent, parent, WIDTH, HEIGHT);
#else
	instance->display = XOpenDisplay(NULL);
	if (instance->display == NULL) {
		free(instance);
		return NULL;
	}
	int s = DefaultScreen(instance->display);
	Window w = XCreateSimpleWindow(instance->display, has_parent ? (Window)parent : RootWindow(instance->display, s), 0, 0, WIDTH, HEIGHT, 0, 0, 0);
	XSelectInput(instance->display, w, ExposureMask | StructureNotifyMask);

	XMapWindow(instance->display, w);
	XSync(instance->display, False);

	instance->widget = (void *)w;

	instance->width = WIDTH;
	instance->height = HEIGHT;
#endif
	instance->cbs = *cbs;
	return instance;
}

static void plugin_ui_free(plugin_ui *instance) {
#if defined(_WIN32) || defined(__CYGWIN__)
#elif defined(__APPLE__)
#else
	XDestroyWindow(instance->display, (Window)instance->widget);
	XCloseDisplay(instance->display);
#endif
	free(instance);
}

static void plugin_ui_idle(plugin_ui *instance) {
#if defined(_WIN32) || defined(__CYGWIN__)
#elif defined(__APPLE__)
#else
	Window w = (Window)instance->widget;
	while (XEventsQueued(instance->display, QueuedAfterFlush) > 0) {
		XEvent e;
		XNextEvent(instance->display, &e);
		if (e.type == Expose) {
			XClearWindow(instance->display, w);
			GC gc = DefaultGC(instance->display, DefaultScreen(instance->display));

			XSetForeground(instance->display, gc, 0xff0000);
			XFillRectangle(instance->display, w, gc, 10, 10, instance->width - 20, instance->height - 20);
		} else if (e.type == ConfigureNotify) {
			XWindowAttributes attrs;
			XGetWindowAttributes(instance->display, w, &attrs);
			instance->width = attrs.width;
			instance->height = attrs.height;
		}
	}
#endif
}

static void plugin_ui_set_parameter(plugin_ui *instance, size_t index, float value) {
	switch (index) {
	case 0:
		instance->gain = 0.0125f * value + 0.75f;
		break;
	case 1:
		instance->delay = 0.001f * value;
		break;
	case 2:
		// (bad) approx log unmap
		instance->cutoff = (1.0326554320337176f * value - 20.65310864067435f) / (value + 632.4555320336754f);
		break;
	case 3:
		instance->bypass = value >= 0.5f;
		break;
	case 4:
		instance->y_z1 = 0.5f * value + 0.5f;
		break;
	}
}
