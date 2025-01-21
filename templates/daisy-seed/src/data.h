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

#define NUM_PARAMETERS			{{=it.product.parameters.length}}

#define NUM_ADC				{{=it.product.parameters.filter((x, i) => x.direction == "input" && it.daisy_seed.parameterPins[i] >= 0).length}}
#define NUM_DAC				{{=it.product.parameters.filter((x, i) => x.direction == "output" && it.daisy_seed.parameterPins[i] >= 0).length}}

#if NUM_PARAMETERS > 0

# define PARAM_BYPASS	1
# define PARAM_TOGGLED	(1<<1)
# define PARAM_INTEGER	(1<<2)
# define PARAM_MAP_LOG	(1<<3)

static struct {
	char		out;
	int8_t		pin;
	float		min;
	float		max;
	float		def;
	uint32_t	flags;
	float		mapK;
} param_data[NUM_PARAMETERS] = {
{{~it.product.parameters :p:i}}
	{
		/* .out		= */ {{=p.direction == "output" ? 1 : 0}},
		/* .pin		= */ {{=it.daisy_seed.parameterPins[i]}},
		/* .min		= */ {{=p.minimum.toExponential()}}f,
		/* .max		= */ {{=p.maximum.toExponential()}}f,
		/* .def		= */ {{=p.defaultValue.toExponential()}}f,
		/* .flags	= */ {{?p.isBypass}}PARAM_BYPASS{{??}}0{{?p.toggled}} | PARAM_TOGGLED{{?}}{{?p.integer}} | PARAM_INTEGER{{?}}{{?p.map == "logarithmic"}} | PARAM_MAP_LOG{{?}}{{?}},
		/* .mapK	= */ {{?p.map == "logarithmic"}}{{=Number(2.0 * Math.log(Math.sqrt(p.maximum * p.minimum) / Math.abs(p.minimum))).toExponential()}}{{??}}0.0{{?}}f
	},
{{~}}
};

# if MIDI_BUS_IN >= 0

#  define HAS_MIDI_CC_MAPS	{{=it.daisy_seed.midiCCMaps ? 1 : 0}}

#  if HAS_MIDI_CC_MAPS
static int midi_cc_maps[NUM_PARAMETERS] = { {{~it.daisy_seed.midiCCMaps :p}}{{=p}} ,{{~}} };
#  endif

# endif

#endif

{{?it.product.state && it.product.state.dspCustom}}
#define STATE_DSP_CUSTOM
{{?}}
