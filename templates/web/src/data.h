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

#define DATA_PRODUCT_AUDIO_INPUT_CHANNELS_N  {{=it.product.buses.filter(x => x.type == "audio" && x.direction == "input").reduce((s, x) => s += x.channels == "mono" ? 1 : 2, 0)}}
#define DATA_PRODUCT_AUDIO_OUTPUT_CHANNELS_N {{=it.product.buses.filter(x => x.type == "audio" && x.direction == "output").reduce((s, x) => s += x.channels == "mono" ? 1 : 2, 0)}}
#define DATA_PRODUCT_MIDI_INPUTS_N           {{=it.product.buses.filter(x => x.type == "midi" && x.direction == "input").length}}
#define DATA_PRODUCT_MIDI_OUTPUTS_N          {{=it.product.buses.filter(x => x.type == "midi" && x.direction == "output").length}}
#define DATA_PRODUCT_PARAMETERS_N            {{=it.product.parameters.length}}
#define DATA_PRODUCT_PARAMETERS_OUTPUT_N     {{=it.product.parameters.filter(x => x.direction == "output").length}}

#if DATA_PRODUCT_PARAMETERS_N > 0
static struct {
	char  out;
	float def;
} param_data[DATA_PRODUCT_PARAMETERS_N] = {
{{~it.product.parameters :p}}
	{
		/* .out	= */ {{=p.direction == "output" ? 1 : 0}},
		/* .def	= */ {{=p.defaultValue.toExponential()}}
	},
{{~}}
};
#endif

#if DATA_PRODUCT_PARAMETERS_OUTPUT_N > 0
static size_t param_out_index[DATA_PRODUCT_PARAMETERS_OUTPUT_N] = {
	{{~it.product.parameters :p:i}}{{?p.direction == "output"}}{{=i}}, {{?}}{{~}}
};
#endif
