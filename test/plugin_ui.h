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

#include <pugl/pugl.h>
#include <pugl/cairo.h>
#include <cairo.h>

typedef struct {
	void *			widget;
	PuglWorld *		world;
	PuglView *		view;

	double			fw;
	double			fh;
	double			x;
	double			y;
	double			w;
	double			h;

	float			gain;
	float			delay;
	float			cutoff;
	char			bypass;
	float			y_z1;

	int			param_down;

	plugin_ui_callbacks	cbs;
} plugin_ui;

#define WIDTH		600.0
#define HEIGHT		400.0
#define RATIO		(WIDTH / HEIGHT)
#define INV_RATIO	(HEIGHT / WIDTH)

static void plugin_ui_get_default_size(uint32_t *width, uint32_t *height) {
	*width = WIDTH;
	*height = HEIGHT;
}

static void plugin_ui_update_geometry(plugin_ui *instance) {
	PuglRect frame = puglGetFrame(instance->view);
	instance->fw = frame.width;
	instance->fh = frame.height;
	if (frame.width == 0 || frame.height == 0)
		return;

	if (instance->fw / instance->fh > RATIO) {
		instance->w = RATIO * instance->fh;
		instance->h = instance->fh;
		instance->x = 0.5 * (instance->fw - instance->w);
		instance->y = 0.0;
	} else {
		instance->w = instance->fw;
		instance->h = INV_RATIO * instance->fw;
		instance->x = 0.0;
		instance->y = 0.5 * (instance->fh - instance->h);
	}
}

static void plugin_ui_draw(plugin_ui *instance) {
	cairo_t *cr = (cairo_t *)puglGetContext(instance->view);
	double x = instance->x;
	double y = instance->y;
	double w = instance->w;
	double h = instance->h;

	cairo_set_line_width(cr, 0.005 * h);

	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_paint(cr);

	cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
	cairo_rectangle(cr, x, y, w, h);
	cairo_fill(cr);

	cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
	cairo_rectangle(cr, x + 0.1 * w, y + 0.15 * h, 0.8 * w, 0.1 * h);
	cairo_fill(cr);
	cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
	cairo_rectangle(cr, x + 0.1 * w, y + 0.15 * h, 0.8 * w * instance->gain, 0.1 * h);
	cairo_fill(cr);
	cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
	cairo_rectangle(cr, x + 0.1 * w, y + 0.15 * h, 0.8 * w, 0.1 * h);
	cairo_stroke(cr);

	cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
	cairo_rectangle(cr, x + 0.1 * w, y + 0.3 * h, 0.8 * w, 0.1 * h);
	cairo_fill(cr);
	cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
	cairo_rectangle(cr, x + 0.1 * w, y + 0.3 * h, 0.8 * w * instance->delay, 0.1 * h);
	cairo_fill(cr);
	cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
	cairo_rectangle(cr, x + 0.1 * w, y + 0.3 * h, 0.8 * w, 0.1 * h);
	cairo_stroke(cr);

	cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
	cairo_rectangle(cr, x + 0.1 * w, y + 0.45 * h, 0.8 * w, 0.1 * h);
	cairo_fill(cr);
	cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
	cairo_rectangle(cr, x + 0.1 * w, y + 0.45 * h, 0.8 * w * instance->cutoff, 0.1 * h);
	cairo_fill(cr);
	cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
	cairo_rectangle(cr, x + 0.1 * w, y + 0.45 * h, 0.8 * w, 0.1 * h);
	cairo_stroke(cr);

	if (instance->bypass)
		cairo_set_source_rgb(cr, 1.0, 0, 0);
	else
		cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
	cairo_rectangle(cr, x + 0.4 * w, y + 0.6 * h, 0.2 * w, 0.1 * h);
	cairo_fill(cr);

	cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
	cairo_rectangle(cr, x + 0.1 * w, y + 0.75 * h, 0.8 * w, 0.1 * h);
	cairo_fill(cr);

	cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
	cairo_rectangle(cr, x + 0.1 * w, y + 0.75 * h, 0.8 * w * instance->y_z1, 0.1 * h);
	cairo_fill(cr);
}

static PuglStatus plugin_ui_on_event(PuglView *view, const PuglEvent *event) {
	switch (event->type) {
		case PUGL_CONFIGURE:
		{
			plugin_ui *instance = (plugin_ui *)puglGetHandle(view);
			plugin_ui_update_geometry(instance);
			puglPostRedisplay(instance->view);
		}
			break;
		case PUGL_BUTTON_PRESS:
		{
			plugin_ui *instance = (plugin_ui *)puglGetHandle(view);
			const PuglButtonEvent *ev = (const PuglButtonEvent *)event;
			double x = instance->x;
			double y = instance->y;
			double w = instance->w;
			double h = instance->h;

			if (ev->x >= x + 0.1 * w && ev->x <= x + 0.9 * w
			    && ev->y >= y + 0.15 * h && ev->y <= y + 0.25 * h) {
				instance->param_down = 0;
				instance->gain = (float)((ev->x - (x + 0.1 * w)) / (0.8 * w));
				instance->cbs.set_parameter_begin(instance->cbs.handle, 0);
				instance->cbs.set_parameter(instance->cbs.handle, 0, -60.f + 80.f * instance->gain);
				puglPostRedisplay(instance->view);
			} else if (ev->x >= x + 0.1 * w && ev->x <= x + 0.9 * w
			    && ev->y >= y + 0.3 * h && ev->y <= y + 0.4 * h) {
				instance->param_down = 1;
				instance->delay = (float)((ev->x - (x + 0.1 * w)) / (0.8 * w));
				instance->cbs.set_parameter_begin(instance->cbs.handle, 1);
				instance->cbs.set_parameter(instance->cbs.handle, 1, 1000.f * instance->delay);
				puglPostRedisplay(instance->view);
			} else if (ev->x >= x + 0.1 * w && ev->x <= x + 0.9 * w
			    && ev->y >= y + 0.45 * h && ev->y <= y + 0.55 * h) {
				instance->param_down = 2;
				instance->cutoff = (float)((ev->x - (x + 0.1 * w)) / (0.8 * w));
				instance->cbs.set_parameter_begin(instance->cbs.handle, 2);
				instance->cbs.set_parameter(instance->cbs.handle, 2, (632.4555320336746f * instance->cutoff + 20.653108640674372f) / (1.0326554320337158f - instance->cutoff));
				puglPostRedisplay(instance->view);
			} else if (ev->x >= x + 0.4 * w && ev->x <= x + 0.6 * w
			    && ev->y >= y + 0.6 * h && ev->y <= y + 0.7 * h) {
				instance->param_down = 3;
			}
		}
			break;
		case PUGL_MOTION:
		{
			plugin_ui *instance = (plugin_ui *)puglGetHandle(view);
			const PuglMotionEvent *ev = (const PuglMotionEvent *)event;
			double x = instance->x;
			double w = instance->w;
			float v = ev->x < x + 0.1 * w ? 0.f : (ev->x > x + 0.9 * w ? 1.f : (float)((ev->x - (x + 0.1 * w)) / (0.8 * w)));

			switch (instance->param_down) {
			case 0:
				instance->gain = v;
				instance->cbs.set_parameter(instance->cbs.handle, 0, -60.f + 80.f * instance->gain);
				puglPostRedisplay(instance->view);
				break;
			case 1:
				instance->delay = v;
				instance->cbs.set_parameter(instance->cbs.handle, 1, 1000.f * instance->delay);
				puglPostRedisplay(instance->view);
				break;
			case 2:
				instance->cutoff = v;
				instance->cbs.set_parameter(instance->cbs.handle, 2, (632.4555320336746f * instance->cutoff + 20.653108640674372f) / (1.0326554320337158f - instance->cutoff));
				puglPostRedisplay(instance->view);
				break;
			}
		}
			break;
		case PUGL_BUTTON_RELEASE:
		{
			plugin_ui *instance = (plugin_ui *)puglGetHandle(view);
			const PuglButtonEvent *ev = (const PuglButtonEvent *)event;
			double x = instance->x;
			double y = instance->y;
			double w = instance->w;
			double h = instance->h;

			if (instance->param_down == 3)
				if (ev->x >= x + 0.4 * w && ev->x <= x + 0.6 * w
				    && ev->y >= y + 0.6 * h && ev->y <= y + 0.7 * h) {
					instance->bypass = !instance->bypass;
					instance->cbs.set_parameter(instance->cbs.handle, 3, instance->bypass ? 1.f : 0.f);
					puglPostRedisplay(instance->view);
				}

			if (instance->param_down != -1) {
				instance->cbs.set_parameter_end(instance->cbs.handle, instance->param_down);
				instance->param_down = -1;
			}
		}
			break;
		case PUGL_EXPOSE:
		{
			plugin_ui *instance = (plugin_ui *)puglGetHandle(view);
			plugin_ui_update_geometry(instance); // I didn't expect this was needed here for X11 to work decently when resizing
			plugin_ui_draw(instance);
		}
			break;
		default:
			break;
	}
	return PUGL_SUCCESS;
}

static plugin_ui *plugin_ui_create(char has_parent, void *parent, plugin_ui_callbacks *cbs) {
	plugin_ui *instance = malloc(sizeof(plugin_ui));
	if (instance == NULL)
		return NULL;
	instance->param_down = -1;
	instance->world = puglNewWorld(PUGL_MODULE, 0);
	instance->view = puglNewView(instance->world);
	puglSetSizeHint(instance->view, PUGL_DEFAULT_SIZE, WIDTH, HEIGHT);
	puglSetViewHint(instance->view, PUGL_RESIZABLE, PUGL_TRUE);
	puglSetHandle(instance->view, instance);
	puglSetBackend(instance->view, puglCairoBackend());
	PuglRect frame = { 0, 0, WIDTH, HEIGHT };
	puglSetFrame(instance->view, frame);
	puglSetEventFunc(instance->view, plugin_ui_on_event);
	if (has_parent)
		puglSetParentWindow(instance->view, (PuglNativeView)parent);
	if (puglRealize(instance->view)) {
		puglFreeView(instance->view);
		puglFreeWorld(instance->world);
		return NULL;
	}
	instance->widget = (void *)puglGetNativeView(instance->view);
	instance->cbs = *cbs;
	puglSetFrame(instance->view, frame); // Intentionally duplicated because of ardour/lv2/mac strange event order call
	puglShow(instance->view, PUGL_SHOW_RAISE); // Cocoa calls events at this so it's better this happens late
	return instance;
}

static void plugin_ui_free(plugin_ui *instance) {
	puglFreeView(instance->view);
	puglFreeWorld(instance->world);
	free(instance);
}

static void plugin_ui_idle(plugin_ui *instance) {
	puglUpdate(instance->world, 0);
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
	puglPostRedisplay(instance->view);
}
