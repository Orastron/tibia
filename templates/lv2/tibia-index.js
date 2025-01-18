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
 * File author: Stefano D'Angelo
 */

var path = require("path");
var sep = path.sep;

module.exports = function (data, api, outputCommon, outputData) {
	if (outputData) {
		data.tibia.lv2 = {
			prefixes: [
				{ id: "atom",	uri: "http://lv2plug.in/ns/ext/atom#" },
				{ id: "doap",	uri: "http://usefulinc.com/ns/doap#" },
				{ id: "foaf",	uri: "http://xmlns.com/foaf/0.1/" },
				{ id: "log",	uri: "http://lv2plug.in/ns/ext/log#" },
				{ id: "lv2",	uri: "http://lv2plug.in/ns/lv2core#" },
				{ id: "midi",	uri: "http://lv2plug.in/ns/ext/midi#" },
				{ id: "pprops",	uri: "http://lv2plug.in/ns/ext/port-props#" },
				{ id: "rdf",	uri: "http://www.w3.org/1999/02/22-rdf-syntax-ns#" },
				{ id: "rdfs",	uri: "http://www.w3.org/2000/01/rdf-schema#" },
				{ id: "state",	uri: "http://lv2plug.in/ns/extensions/state#" },
				{ id: "ui",	uri: "http://lv2plug.in/ns/extensions/ui#" },
				{ id: "units",	uri: "http://lv2plug.in/ns/extensions/units#" },
				{ id: "urid",	uri: "http://lv2plug.in/ns/ext/urid#" }
			],
			units: {
				"bar":			"@units:bar",
				"beat":			"@units:beat",
				"bpm":			"@units:bpm",
				"cent":			"@units:cent",
				"cm":			"@units:cm",
				"coef":			"@units:coef",
				"db":			"@units:db",
				"degree":		"@units:degree",
				"frame":		"@units:frame",
				"hz":			"@units:hz",
				"inch":			"@units:inch",
				"khz":			"@units:khz",
				"km":			"@units:km",
				"m":			"@units:m",
				"mhz":			"@units:mhz",
				"midiNote":		"@units:midiNote",
				"mile":			"@units:mile",
				"min":			"@units:min",
				"mm":			"@units:mm",
				"ms":			"@units:ms",
				"oct":			"@units:oct",
				"pc":			"@units:pc",
				"s":			"@units:s",
				"semitone12TET":	"@units:semitone12TET"
			},
			ports: [],

			ttlURI: function (uri) {
				if (uri.charAt(0) == "@") {
					if (uri.indexOf("#") == -1)
						return uri.substring(1);
					uri = data.tibia.lv2.expandURI(uri);
				}
				return "<" + uri + ">";
			},

			expandURI: function (uri) {
				if (uri.charAt(0) != "@")
					return uri;
				var i = uri.indexOf(":");
				var p = uri.substring(1, uri.indexOf(":"));
				return data.tibia.lv2.prefixes.find(x => x.id == p).uri + uri.substring(i + 1);
			}
		};

		for (var id in data.lv2.prefixes)
			data.tibia.lv2.prefixes.push({ id: id, uri: data.lv2.prefixes[id] });

		data.tibia.lv2.prefixes.push({ id: "plugin", uri: data.tibia.lv2.expandURI(data.lv2.uri) + "#" });

		var buses = data.product.buses;
		var audioPorts = [];
		var midiPorts = [];
		for (var bi = 0; bi < buses.length; bi++) {
			var b = buses[bi];
			if (b.type == "audio") {
				if (b.channels == "mono") {
					var e = { type: "audio", direction: b.direction, name: b.name, sidechain: b.sidechain, cv: b.cv, optional: b.optional, busIndex: bi, id: b.id };
					audioPorts.push(e);
				} else {
					var e = { type: "audio", direction: b.direction, name: b.name + " Left", shortName: b.shortName + " L", sidechain: b.sidechain, cv: b.cv, busIndex: bi, id: b.id + "_l" };
					audioPorts.push(e);
					var e = { type: "audio", direction: b.direction, name: b.name + " Right", shortName: b.shortName + " R", sidechain: b.sidechain, cv: b.cv, busIndex: bi, id: b.id + "_r" };
					audioPorts.push(e);
				}
			} else {
				var e =	{ type: "midi", direction: b.direction, name: b.name, sidechain: b.sidechain, control: b.control, optional: b.optional, busIndex: bi, id: b.id };
				midiPorts.push(e);
			}
		}
		audioPorts.sort((a, b) => a.direction != b.direction ? (a.direction == "input" ? -1 : 1) : 0);
		midiPorts.sort((a, b) => a.direction != b.direction ? (a.direction == "input" ? -1 : 1) : 0);
		data.tibia.lv2.ports.push.apply(data.tibia.lv2.ports, audioPorts);
		data.tibia.lv2.ports.push.apply(data.tibia.lv2.ports, midiPorts);

		var ports = [];
		for (var i = 0; i < data.product.parameters.length; i++) {
			var p = data.product.parameters[i];
			var e = Object.create(p);
			e.type = "control";
			e.paramIndex = i;
			ports.push(e);
		}
		ports.sort((a, b) => a.direction != b.direction ? (a.direction == "input" ? -1 : 1) : 0);
		data.tibia.lv2.ports.push.apply(data.tibia.lv2.ports, ports);
	}

	api.generateFileFromTemplateFile(`data${sep}manifest.ttl.in`, `data${sep}manifest.ttl.in`, data);
	api.copyFile(`src${sep}lv2.c`, `src${sep}lv2.c`);
	api.generateFileFromTemplateFile(`src${sep}data.h`, `src${sep}data.h`, data);
};
