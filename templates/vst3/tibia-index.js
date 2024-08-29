/*
 * Tibia
 *
 * Copyright (C) 2023, 2024 Orastron Srl unipersonale
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

var path = require("path");
var sep = path.sep;

module.exports = function (data, api, outputCommon, outputData) {
	if (outputData) {
		data.tibia.vst3 = {
			units: {
				"bar":			"bars",
				"beat":			"beats",
				"bpm":			"BPM",
				"cent":			"ct",
				"cm":			"cm",
				"coef":			"",
				"db":			"dB",
				"degree":		"deg",
				"frame":		"frames",
				"hz":			"Hz",
				"inch":			"\"",
				"khz":			"kHz",
				"km":			"km",
				"m":			"m",
				"mhz":			"MHz",
				"midiNote":		"MIDI note",
				"mile":			"mi",
				"min":			"mins",
				"mm":			"mm",
				"ms":			"ms",
				"oct":			"octaves",
				"pc":			"%",
				"s":			"s",
				"semitone12TET":	"semi"
			},

			sdbm: function (s) {
				var hash = 0;
				for (var i = 0; i < s.length; i++)
					hash = s.charCodeAt(i) + (hash << 6) + (hash << 16) - hash;
				return hash >>> 0;
			}
		};

		for (var i = 0; i < data.product.parameters.length; i++)
			data.product.parameters[i].paramIndex = i;
	}

	api.copyFile(`data${sep}PkgInfo`, `data${sep}PkgInfo`);
	api.generateFileFromTemplateFile(`data${sep}Info.plist`, `data${sep}Info.plist`, data);
	api.copyFile(`src${sep}vst3.c`, `src${sep}vst3.c`);
	api.generateFileFromTemplateFile(`src${sep}data.h`, `src${sep}data.h`, data);
	api.copyFile(`..${sep}common${sep}fatica.h`, `src${sep}fatica.h`);
};
