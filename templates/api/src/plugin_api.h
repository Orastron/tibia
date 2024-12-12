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

#ifndef PLUGIN_API_H
#define PLUGIN_API_H

typedef struct {
	void *		handle;
	const char *	format;
	const char * (*get_bindir)(void *handle);
	const char * (*get_datadir)(void *handle);
} plugin_callbacks;

typedef struct {
	void *		handle;
	const char *	format;
	const char * (*get_bindir)(void *handle);
	const char * (*get_datadir)(void *handle);
	void (*set_parameter_begin)(void *handle, size_t index);
	void (*set_parameter)(void *handle, size_t index, float value);
	void (*set_parameter_end)(void *handle, size_t index);
} plugin_ui_callbacks;

{{?it.product.parameters.length > 0}}
enum {
	{{~it.product.parameters :p}}
	plugin_parameter_{{=p.id}},
	{{~}}
	plugin_parameter__count
};
{{?}}

#endif
