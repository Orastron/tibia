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

var path = require("path");
var sep = path.sep;

module.exports = function (data, api) {
	api.copyFile(`src${sep}string.h`, `src${sep}string.h`);
	api.copyFile(`src${sep}string.c`, `src${sep}string.c`);
	api.copyFile(`src${sep}walloc.h`, `src${sep}walloc.h`);
	api.copyFile(`src${sep}walloc.c`, `src${sep}walloc.c`);
	api.copyFile(`src${sep}new.cpp`, `src${sep}new.cpp`);
	api.copyFile(`src${sep}processor.c`, `src${sep}processor.c`);
	api.generateFileFromTemplateFile(`src${sep}data.h`, `src${sep}data.h`, data);
	api.generateFileFromTemplateFile(`src${sep}processor.js`, `src${sep}processor.js`, data);
	api.generateFileFromTemplateFile(`src${sep}module.js`, `src${sep}module.js`, data);
	api.copyFile(`..${sep}common${sep}callbacks.h`, `src${sep}callbacks.h`);
};
