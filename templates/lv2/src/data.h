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

#define DATA_LV2_URI				"{{=it.tibia.CGetUTF8StringLiteral(it.tibia.lv2.expandURI(it.lv2.uri))}}"

#define DATA_PRODUCT_AUDIO_INPUT_CHANNELS_N	{{=it.product.buses.filter(x => x.type == "audio" && x.direction == "input").reduce((s, x) => s += x.channels == "mono" ? 1 : 2, 0)}}
#define DATA_PRODUCT_AUDIO_OUTPUT_CHANNELS_N	{{=it.product.buses.filter(x => x.type == "audio" && x.direction == "output").reduce((s, x) => s += x.channels == "mono" ? 1 : 2, 0)}}
#define DATA_PRODUCT_MIDI_INPUTS_N		{{=it.product.buses.filter(x => x.type == "midi" && x.direction == "input").length}}
#define DATA_PRODUCT_MIDI_OUTPUTS_N		{{=it.product.buses.filter(x => x.type == "midi" && x.direction == "output").length}}
#define DATA_PRODUCT_CONTROL_INPUTS_N		{{=it.product.parameters.filter(x => x.direction == "input").length}}
#define DATA_PRODUCT_CONTROL_OUTPUTS_N		{{=it.product.parameters.filter(x => x.direction == "output").length}}

{{?it.product.buses.find(x => x.type == "midi" && !x.optional)}}
#define DATA_PRODUCT_MIDI_REQUIRED
{{?}}

#if DATA_PRODUCT_MIDI_INPUTS_N > 0
static uint32_t midi_in_index[DATA_PRODUCT_MIDI_INPUTS_N] = {
	{{~it.tibia.lv2.ports.filter(x => x.type == "midi" && x.direction == "input") :p}}{{=p.busIndex}}, {{~}}
};
#endif

# define DATA_PARAM_BYPASS	1
# define DATA_PARAM_TOGGLED	(1<<1)
# define DATA_PARAM_INTEGER	(1<<2)

{{?it.lv2.use_parameters}}
#define DATA_PRODUCT_USE_PARAMETERS 1

{{?it.tibia.lv2.ports.find(p => p.isInputParameterMessage)}}
#define DATA_PRODUCT_IPM {{=it.tibia.lv2.ports.indexOf(it.tibia.lv2.ports.find(p => p.isInputParameterMessage))}}
{{?}}
{{?it.tibia.lv2.ports.find(p => p.isOutputParameterMessage)}}
#define DATA_PRODUCT_OPM {{=it.tibia.lv2.ports.indexOf(it.tibia.lv2.ports.find(p => p.isOutputParameterMessage))}}
{{?}}


#if DATA_PRODUCT_CONTROL_INPUTS_N + DATA_PRODUCT_CONTROL_OUTPUTS_N > 0

static struct {
	uint32_t	index;
	float		min;
	float		max;
	float		def;
	uint32_t	flags;
	const char *id;
} param_data[DATA_PRODUCT_CONTROL_INPUTS_N + DATA_PRODUCT_CONTROL_OUTPUTS_N] = {
	{{~it.tibia.lv2.parameters :p}}
	{
		/* .index	= */ {{=p.paramIndex}},
		/* .min		= */ {{=p.minimum.toExponential()}}f,
		/* .max		= */ {{=p.maximum.toExponential()}}f,
		/* .def		= */ {{=p.defaultValue.toExponential()}}f,
		/* .flags	= */ {{?p.isBypass}}DATA_PARAM_BYPASS{{??p.isLatency}}DATA_PARAM_INTEGER{{??}}0{{?p.toggled}} | DATA_PARAM_TOGGLED{{?}}{{?p.integer}} | DATA_PARAM_INTEGER{{?}}{{?}},
		/* .id      = */ DATA_LV2_URI"#{{=p.id}}"
	},
	{{~}}
};

#endif

#if DATA_PRODUCT_CONTROL_OUTPUTS_N > 0
static uint32_t param_out_index[DATA_PRODUCT_CONTROL_OUTPUTS_N] = {
	{{~it.tibia.lv2.parameters.filter(x => x.direction == "output") :p}}{{=p.paramIndex}}, {{~}}
};
#endif


{{??}}

#if DATA_PRODUCT_CONTROL_INPUTS_N > 0

static struct {
	uint32_t	index;
	float		min;
	float		max;
	float		def;
	uint32_t	flags;
} param_data[DATA_PRODUCT_CONTROL_INPUTS_N] = {
	{{~it.tibia.lv2.ports.filter(x => x.type == "control" && x.direction == "input") :p}}
	{
		/* .index	= */ {{=p.paramIndex}},
		/* .min		= */ {{=p.minimum.toExponential()}}f,
		/* .max		= */ {{=p.maximum.toExponential()}}f,
		/* .def		= */ {{=p.defaultValue.toExponential()}}f,
		/* .flags	= */ {{?p.isBypass}}DATA_PARAM_BYPASS{{??p.isLatency}}DATA_PARAM_INTEGER{{??}}0{{?p.toggled}} | DATA_PARAM_TOGGLED{{?}}{{?p.integer}} | DATA_PARAM_INTEGER{{?}}{{?}}
	},
	{{~}}
};

#endif

#if DATA_PRODUCT_CONTROL_OUTPUTS_N > 0
static uint32_t param_out_index[DATA_PRODUCT_CONTROL_OUTPUTS_N] = {
	{{~it.tibia.lv2.ports.filter(x => x.type == "control" && x.direction == "output") :p}}{{=p.paramIndex}}, {{~}}
};
#endif

{{?}}

{{?it.product.ui}}
#define DATA_UI
#define DATA_LV2_UI_URI				"{{=it.tibia.CGetUTF8StringLiteral(it.tibia.lv2.expandURI(it.lv2.uri + '#ui'))}}"
#define DATA_UI_USER_RESIZABLE			{{=it.product.ui.userResizable ? 1 : 0}}
{{?}}

{{?(it.product.ui || (it.product.state && it.product.state.dspCustom))}}
#if DATA_PRODUCT_CONTROL_INPUTS_N > 0
static uint32_t index_to_param[DATA_PRODUCT_CONTROL_INPUTS_N + DATA_PRODUCT_CONTROL_OUTPUTS_N] = {
	{{~it.tibia.lv2.ports.filter(x => x.type == "control").map((e, i) => ({ i: i, pi: e.paramIndex })).sort((a, b) => a.pi - b.pi) :p}}{{=p.i + it.tibia.lv2.ports.filter(x => x.type != "control").length}}, {{~}}
};
#endif
{{?}}

{{?it.product.state && it.product.state.dspCustom}}
#define DATA_STATE_DSP_CUSTOM
{{?}}
