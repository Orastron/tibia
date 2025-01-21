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

#define NUM_AUDIO_BUSES_IN		{{=it.product.buses.filter(x => x.type == "audio" && x.direction == "input").length}}
#define NUM_AUDIO_BUSES_OUT		{{=it.product.buses.filter(x => x.type == "audio" && x.direction == "output").length}}

#define AUDIO_BUS_IN			{{=it.product.buses.findIndex(x => x.type == "audio" && x.direction == "input" && !x.cv && !x.sidechain && !x.optional)}}
#define AUDIO_BUS_OUT			{{=it.product.buses.findIndex(x => x.type == "audio" && x.direction == "output" && !x.cv && !x.sidechain && !x.optional)}}

#define NUM_CHANNELS_IN			{{=it.product.buses.filter(x => x.type == "audio" && x.direction == "input" && !x.cv && !x.sidechain && !x.optional).length > 0 ? (it.product.buses.filter(x => x.type == "audio" && x.direction == "input" && !x.cv && !x.sidechain && !x.optional)[0].channels == "mono" ? 1 : 2) : 0}}
#define NUM_CHANNELS_OUT		{{=it.product.buses.filter(x => x.type == "audio" && x.direction == "output" && !x.cv && !x.sidechain && !x.optional).length > 0 ? (it.product.buses.filter(x => x.type == "audio" && x.direction == "output" && !x.cv && !x.sidechain && !x.optional)[0].channels == "mono" ? 1 : 2) : 0}}
#define NUM_NON_OPT_CHANNELS_IN		{{=it.product.buses.filter(x => x.type == "audio" && x.direction == "input" && !x.optional).reduce((a, x) => a + (x.channels == "mono" ? 1 : 2), 0)}}
#define NUM_NON_OPT_CHANNELS_OUT	{{=it.product.buses.filter(x => x.type == "audio" && x.direction == "output" && !x.optional).reduce((a, x) => a + (x.channels == "mono" ? 1 : 2), 0)}}
#define NUM_ALL_CHANNELS_IN		{{=it.product.buses.filter(x => x.type == "audio" && x.direction == "input").reduce((a, x) => a + (x.channels == "mono" ? 1 : 2), 0)}}
#define NUM_ALL_CHANNELS_OUT		{{=it.product.buses.filter(x => x.type == "audio" && x.direction == "output").reduce((a, x) => a + (x.channels == "mono" ? 1 : 2), 0)}}

#define NUM_MIDI_INPUTS			{{=it.product.buses.filter(x => x.type == "midi" && x.direction == "input").length}}

#define MIDI_BUS_IN			{{=it.product.buses.findIndex(x => x.type == "midi" && x.direction == "input")}}

#if NUM_AUDIO_BUSES_IN + NUM_AUDIO_BUSES_OUT > 0
static struct {
	size_t	index;
	char	out;
	char	optional;
	char	channels;
} audio_bus_data[NUM_AUDIO_BUSES_IN + NUM_AUDIO_BUSES_OUT] = {
{{~it.product.buses :b:i}}
{{?b.type == "audio"}}
	{
		/* .index	= */ {{=i}},
		/* .out		= */ {{=b.direction == "output" ? 1 : 0}},
		/* .optional	= */ {{=b.optional ? 1 : 0}},
		/* .channels	= */ {{=b.channels == "mono" ? 1 : 2}}
	},
{{?}}
{{~}}
};
#endif

#define PARAMETERS_N			{{=it.product.parameters.length}}

#if PARAMETERS_N > 0

# define PARAM_BYPASS	1
# define PARAM_TOGGLED	(1<<1)
# define PARAM_INTEGER	(1<<2)

static struct {
	const char *	id;
	char		out;
	float		def;
	float		min;
	float		max;
	uint32_t	flags;
} param_data[PARAMETERS_N] = {
{{~it.product.parameters :p:i}}
	{
		/* .id		= */ "{{=p.id}}",
		/* .out		= */ {{=p.direction == "output" ? 1 : 0}},
		/* .def		= */ {{=p.defaultValue.toExponential()}}f,
		/* .min		= */ {{=p.minimum.toExponential()}}f,
		/* .max		= */ {{=p.maximum.toExponential()}}f,
		/* .flags	= */ {{?p.isBypass}}PARAM_BYPASS{{??}}0{{?p.toggled}} | PARAM_TOGGLED{{?}}{{?p.integer}} | PARAM_INTEGER{{?}}{{?}}
	},
{{~}}
};
#endif

{{?it.product.state && it.product.state.dspCustom}}
#define STATE_DSP_CUSTOM
{{?}}
