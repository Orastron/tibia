/*
 * Tibia
 *
 * Copyright (C) 2023-2025 Orastron Srl unipersonale
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

#define DATA_COMPANY_NAME			"{{=it.tibia.CGetUTF8StringLiteral(it.company.name, 63)}}"
#define DATA_COMPANY_URL			"{{=it.tibia.CGetUTF8StringLiteral(it.company.url, 255)}}"
#define DATA_COMPANY_EMAIL			"{{=it.tibia.CGetUTF8StringLiteral(it.company.email, 127)}}"

#define DATA_PRODUCT_NAME			"{{=it.tibia.CGetUTF8StringLiteral(it.product.name, 63)}}"
#define DATA_PRODUCT_VERSION			"{{=(it.product.version + '.' + it.product.buildVersion).substring(0, 63)}}"

static Steinberg_char16 dataProductNameW[64] = { {{~Array.from(it.product.name).slice(0, 63) :c}}0x{{=c.charCodeAt(0).toString(16)}}, {{~}}0 };
static Steinberg_char16 dataProductVersionW[64] = { {{~Array.from(it.product.version + "." + it.product.buildVersion).slice(0, 63) :c}}0x{{=c.charCodeAt(0).toString(16)}}, {{~}}0 };

#define DATA_VST3_SDK_VERSION			"VST 3.7.13"
static Steinberg_char16 dataVST3SDKVersionW[64] = { {{~Array.from("VST 3.7.13") :c}}0x{{=c.charCodeAt(0).toString(16)}}, {{~}}0 };

static Steinberg_char16 dataVST3ControllerNameW[64] = { {{~Array.from(it.product.name + " Controller").slice(0, 63) :c}}0x{{=c.charCodeAt(0).toString(16)}}, {{~}}0 };

#define DATA_VST3_PLUGIN_CID_1			0x{{=it.vst3.plugin.cid.substring(0, 8)}}
#define DATA_VST3_PLUGIN_CID_2			0x{{=it.vst3.plugin.cid.substring(8, 16)}}
#define DATA_VST3_PLUGIN_CID_3			0x{{=it.vst3.plugin.cid.substring(16, 24)}}
#define DATA_VST3_PLUGIN_CID_4			0x{{=it.vst3.plugin.cid.substring(24, 32)}}

#define DATA_VST3_CONTROLLER_CID_1		0x{{=it.vst3.controller.cid.substring(0, 8)}}
#define DATA_VST3_CONTROLLER_CID_2		0x{{=it.vst3.controller.cid.substring(8, 16)}}
#define DATA_VST3_CONTROLLER_CID_3		0x{{=it.vst3.controller.cid.substring(16, 24)}}
#define DATA_VST3_CONTROLLER_CID_4		0x{{=it.vst3.controller.cid.substring(24, 32)}}

static const Steinberg_TUID dataPluginCID = SMTG_INLINE_UID(DATA_VST3_PLUGIN_CID_1, DATA_VST3_PLUGIN_CID_2, DATA_VST3_PLUGIN_CID_3, DATA_VST3_PLUGIN_CID_4);
static const Steinberg_TUID dataControllerCID = SMTG_INLINE_UID(DATA_VST3_CONTROLLER_CID_1, DATA_VST3_CONTROLLER_CID_2, DATA_VST3_CONTROLLER_CID_3, DATA_VST3_CONTROLLER_CID_4);

#define DATA_VST3_SUBCATEGORY			"{{=it.vst3.subCategory}}"

#define DATA_PRODUCT_BUSES_AUDIO_INPUT_N	{{=it.product.buses.filter(x => x.type == "audio" && x.direction == "input").length}}
#define DATA_PRODUCT_BUSES_AUDIO_OUTPUT_N	{{=it.product.buses.filter(x => x.type == "audio" && x.direction == "output").length}}
#define DATA_PRODUCT_BUSES_MIDI_INPUT_N		{{=it.product.buses.filter(x => x.type == "midi" && x.direction == "input").length}}
#define DATA_PRODUCT_BUSES_MIDI_OUTPUT_N	{{=it.product.buses.filter(x => x.type == "midi" && x.direction == "output").length}}

#define DATA_PRODUCT_CHANNELS_AUDIO_INPUT_N	{{=it.product.buses.filter(x => x.type == "audio" && x.direction == "input").reduce((a, x) => a + (x.channels == "mono" ? 1 : 2), 0)}}
#define DATA_PRODUCT_CHANNELS_AUDIO_OUTPUT_N	{{=it.product.buses.filter(x => x.type == "audio" && x.direction == "output").reduce((a, x) => a + (x.channels == "mono" ? 1 : 2), 0)}}

#if DATA_PRODUCT_BUSES_AUDIO_INPUT_N > 0
static struct Steinberg_Vst_BusInfo busInfoAudioInput[DATA_PRODUCT_BUSES_AUDIO_INPUT_N] = {
{{~it.product.buses.filter(x => x.type == "audio" && x.direction == "input") :b}}
	{
		/* .mediaType		= */ Steinberg_Vst_MediaTypes_kAudio,
		/* .direction		= */ Steinberg_Vst_BusDirections_kInput,
		/* .channelCount	= */ {{=b.channels == "mono" ? 1 : 2}},
		/* .name		= */ { {{~Array.from(b.name) :c}}0x{{=c.charCodeAt(0).toString(16)}}, {{~}}0 },
		/* .busType		= */ {{?b.sidechain}}Steinberg_Vst_BusTypes_kAux{{??}}Steinberg_Vst_BusTypes_kMain{{?}},
		/* .flags		= */ 0{{?b.cv}} | Steinberg_Vst_BusInfo_BusFlags_kIsControlVoltage{{?}}{{?!b.optional}} | Steinberg_Vst_BusInfo_BusFlags_kDefaultActive{{?}}
	},
{{~}}
};
#endif

#if DATA_PRODUCT_BUSES_AUDIO_OUTPUT_N > 0
static struct Steinberg_Vst_BusInfo busInfoAudioOutput[DATA_PRODUCT_BUSES_AUDIO_OUTPUT_N] = {
{{~it.product.buses.filter(x => x.type == "audio" && x.direction == "output") :b}}
	{
		/* .mediaType		= */ Steinberg_Vst_MediaTypes_kAudio,
		/* .direction		= */ Steinberg_Vst_BusDirections_kOutput,
		/* .channelCount	= */ {{=b.channels == "mono" ? 1 : 2}},
		/* .name		= */ { {{~Array.from(b.name) :c}}0x{{=c.charCodeAt(0).toString(16)}}, {{~}}0 },
		/* .busType		= */ {{?b.sidechain}}Steinberg_Vst_BusTypes_kAux{{??}}Steinberg_Vst_BusTypes_kMain{{?}},
		/* .flags		= */ 0{{?b.cv}} | Steinberg_Vst_BusInfo_BusFlags_kIsControlVoltage{{?}}{{?!b.optional}} | Steinberg_Vst_BusInfo_BusFlags_kDefaultActive{{?}}
	},
{{~}}
};
#endif

#if DATA_PRODUCT_BUSES_MIDI_INPUT_N > 0
static struct Steinberg_Vst_BusInfo busInfoMidiInput[DATA_PRODUCT_BUSES_MIDI_INPUT_N] = {
{{~it.product.buses.filter(x => x.type == "midi" && x.direction == "input") :b}}
	{
		/* .mediaType		= */ Steinberg_Vst_MediaTypes_kEvent,
		/* .direction		= */ Steinberg_Vst_BusDirections_kInput,
		/* .channelCount	= */ 16,
		/* .name		= */ { {{~Array.from(b.name) :c}}0x{{=c.charCodeAt(0).toString(16)}}, {{~}}0 },
		/* .busType		= */ {{?b.sidechain}}Steinberg_Vst_BusTypes_kAux{{??}}Steinberg_Vst_BusTypes_kMain{{?}},
		/* .flags		= */ 0{{?!b.optional}} | Steinberg_Vst_BusInfo_BusFlags_kDefaultActive{{?}}
	},
{{~}}
};
#endif

#if DATA_PRODUCT_BUSES_MIDI_OUTPUT_N > 0
static struct Steinberg_Vst_BusInfo busInfoMidiOutput[DATA_PRODUCT_BUSES_MIDI_OUTPUT_N] = {
{{~it.product.buses.filter(x => x.type == "midi" && x.direction == "output") :b}}
	{
		/* .mediaType		= */ Steinberg_Vst_MediaTypes_kEvent,
		/* .direction		= */ Steinberg_Vst_BusDirections_kOutput,
		/* .channelCount	= */ 16,
		/* .name		= */ { {{~Array.from(b.name) :c}}0x{{=c.charCodeAt(0).toString(16)}}, {{~}}0 },
		/* .busType		= */ {{?b.sidechain}}Steinberg_Vst_BusTypes_kAux{{??}}Steinberg_Vst_BusTypes_kMain{{?}},
		/* .flags		= */ 0{{?!b.optional}} | Steinberg_Vst_BusInfo_BusFlags_kDefaultActive{{?}}
	},
{{~}}
};
#endif

#if DATA_PRODUCT_BUSES_MIDI_INPUT_N > 0
static uint32_t midiInIndex[DATA_PRODUCT_BUSES_MIDI_INPUT_N] = {
	{{~it.product.buses :b:i}}{{?b.type == "midi" && b.direction == "input"}}{{=i}}, {{?}}{{~}}
};
#endif

#define DATA_PRODUCT_PARAMETERS_N		{{=it.product.parameters.filter(x => !x.isLatency).length}}
#define DATA_PRODUCT_PARAMETERS_IN_N		{{=it.product.parameters.filter(x => x.direction == "input").length}}
#define DATA_PRODUCT_PARAMETERS_OUT_N		{{=it.product.parameters.filter(x => !x.isLatency && x.direction == "output").length}}

#if DATA_PRODUCT_PARAMETERS_N + DATA_PRODUCT_BUSES_MIDI_INPUT_N > 0
static struct Steinberg_Vst_ParameterInfo parameterInfo[DATA_PRODUCT_PARAMETERS_N + 3 * DATA_PRODUCT_BUSES_MIDI_INPUT_N] = {
{{~it.product.parameters.filter(x => !x.isLatency) :p:i}}
	{
{{?p.isBypass}}
		/* .id				= */ {{=(it.tibia.sdbm("bypass ") & 0x7fffffff) >>> 0}},
		/* .title			= */ { {{~Array.from("Bypass") :c}}0x{{=c.charCodeAt(0).toString(16)}}, {{~}}0 },
		/* .shortTitle			= */ { {{~Array.from("Bypass") :c}}0x{{=c.charCodeAt(0).toString(16)}}, {{~}}0 },
		/* .units			= */ { 0 },
		/* .stepCount			= */ 1,
		/* .defaultNormalizedValue	= */ 0.0,
		/* .unitId			= */ 0,
		/* .flags			= */ Steinberg_Vst_ParameterInfo_ParameterFlags_kIsBypass | Steinberg_Vst_ParameterInfo_ParameterFlags_kCanAutomate
{{??}}
		/* .id				= */ {{=(it.tibia.sdbm(p.id) & 0x7fffffff) >>> 0}},
		/* .title			= */ { {{~Array.from(p.name) :c}}0x{{=c.charCodeAt(0).toString(16)}}, {{~}}0 },
		/* .shortTitle			= */ { {{~Array.from(p.shortName) :c}}0x{{=c.charCodeAt(0).toString(16)}}, {{~}}0 },
		/* .units			= */ { {{~Array.from(p.unit in it.tibia.vst3.units ? it.tibia.vst3.units[p.unit] : "") :c}}0x{{=c.charCodeAt(0).toString(16)}}, {{~}}0 },
		/* .stepCount			= */ {{=p.toggled ? 1 : (p.list && p.scalePoints.length > 1 ? p.scalePoints.length - 1 : (p.integer ? p.maximum - p.minimum : 0))}},
		/* .defaultNormalizedValue	= */ {{?p.map == "logarithmic"}}{{=Number(Math.log(p.defaultValue / p.minimum) / (2 * Math.log(Math.sqrt(p.minimum * p.maximum) / Math.abs(p.minimum)))).toExponential()}}{{??}}{{=Number((p.defaultValue - p.minimum) / (p.maximum - p.minimum)).toExponential()}}{{?}},
		/* .unitId			= */ 0,
		/* .flags			= */ {{?p.direction == "input"}}Steinberg_Vst_ParameterInfo_ParameterFlags_kCanAutomate{{??}}Steinberg_Vst_ParameterInfo_ParameterFlags_kIsReadOnly{{?}}
{{?}}
	},
{{~}}
{{~it.product.buses.filter(x => x.type == "midi" && x.direction == "input") :b:i}}
	{
		/* .id				= */ {{=(it.tibia.sdbm(b.id + " channel pressure") & 0x7fffffff) >>> 0}},
		/* .title			= */ { {{~Array.from(b.name + " Channel Pressure").slice(0, 127) :c}}0x{{=c.charCodeAt(0).toString(16)}}, {{~}}0 },
		/* .shortTitle			= */ { {{~Array.from(b.shortName + " Chan Pres").slice(0, 127) :c}}0x{{=c.charCodeAt(0).toString(16)}}, {{~}}0 },
		/* .units			= */ { 0 },
		/* .stepCount			= */ 0,
		/* .defaultNormalizedValue	= */ 0.0,
		/* .unitId			= */ 0,
		/* .flags			= */ Steinberg_Vst_ParameterInfo_ParameterFlags_kIsHidden | Steinberg_Vst_ParameterInfo_ParameterFlags_kCanAutomate
	},
	{
		/* .id				= */ {{=(it.tibia.sdbm(b.id + " pitch bend") & 0x7fffffff) >>> 0}},
		/* .title			= */ { {{~Array.from(b.name + " Pitch Bend").slice(0, 127) :c}}0x{{=c.charCodeAt(0).toString(16)}}, {{~}}0 },
		/* .shortTitle			= */ { {{~Array.from(b.shortName + " Pitch Bend").slice(0, 127) :c}}0x{{=c.charCodeAt(0).toString(16)}}, {{~}}0 },
		/* .units			= */ { 0 },
		/* .stepCount			= */ 0,
		/* .defaultNormalizedValue	= */ 0.5,
		/* .unitId			= */ 0,
		/* .flags			= */ Steinberg_Vst_ParameterInfo_ParameterFlags_kIsHidden | Steinberg_Vst_ParameterInfo_ParameterFlags_kCanAutomate
	},
	{
		/* .id				= */ {{=(it.tibia.sdbm(b.id + " mod wheel") & 0x7fffffff) >>> 0}},
		/* .title			= */ { {{~Array.from(b.name + " Mod Wheel").slice(0, 127) :c}}0x{{=c.charCodeAt(0).toString(16)}}, {{~}}0 },
		/* .shortTitle			= */ { {{~Array.from(b.shortName + " Mod Wheel").slice(0, 127) :c}}0x{{=c.charCodeAt(0).toString(16)}}, {{~}}0 },
		/* .units			= */ { 0 },
		/* .stepCount			= */ 0,
		/* .defaultNormalizedValue	= */ 0.0,
		/* .unitId			= */ 0,
		/* .flags			= */ Steinberg_Vst_ParameterInfo_ParameterFlags_kIsHidden | Steinberg_Vst_ParameterInfo_ParameterFlags_kCanAutomate
	},
{{~}}
};

# if DATA_PRODUCT_PARAMETERS_N > 0

#  define DATA_PARAM_BYPASS	1
#  define DATA_PARAM_TOGGLED	(1<<1)
#  define DATA_PARAM_INTEGER	(1<<2)
#  define DATA_PARAM_MAP_LOG	(1<<3)

typedef struct {
	size_t		index;
	double		min;
	double		max;
	double		def;
	uint32_t	flags;
	double		mapK;
	// scalePoints?
} ParameterData;

#  if DATA_PRODUCT_PARAMETERS_IN_N > 0
static ParameterData parameterInData[DATA_PRODUCT_PARAMETERS_IN_N] = {
{{~it.product.parameters.filter(x => x.direction == "input") :p:i}}
	{
		/* .index	= */ {{=p.paramIndex}},
		/* .min		= */ {{=p.minimum.toExponential()}},
		/* .max		= */ {{=p.maximum.toExponential()}},
		/* .def		= */ {{=p.defaultValue.toExponential()}},
		/* .flags	= */ {{?p.isBypass}}DATA_PARAM_BYPASS{{??}}0{{?p.toggled}} | DATA_PARAM_TOGGLED{{?}}{{?p.integer}} | DATA_PARAM_INTEGER{{?}}{{?p.map == "logarithmic"}} | DATA_PARAM_MAP_LOG{{?}}{{?}},
		/* .mapK	= */ {{?p.map == "logarithmic"}}{{=Number(2.0 * Math.log(Math.sqrt(p.maximum * p.minimum) / Math.abs(p.minimum))).toExponential()}}{{??}}0.0{{?}}
	},
{{~}}
};
#  endif

#  if DATA_PRODUCT_PARAMETERS_OUT_N > 0
static ParameterData parameterOutData[DATA_PRODUCT_PARAMETERS_OUT_N] = {
{{~it.product.parameters.filter(x => !x.isLatency && x.direction == "output") :p:i}}
	{
		/* .index	= */ {{=p.paramIndex}},
		/* .min		= */ {{=p.minimum.toExponential()}},
		/* .max		= */ {{=p.maximum.toExponential()}},
		/* .def		= */ {{=p.defaultValue.toExponential()}},
		/* .flags	= */ 0{{?p.toggled}} | DATA_PARAM_TOGGLED{{?}}{{?p.integer}} | DATA_PARAM_INTEGER{{?}}{{?p.map == "logarithmic"}} | DATA_PARAM_MAP_LOG{{?}},
		/* .mapK	= */ {{?p.map == "logarithmic"}}{{=Number(2.0 * Math.log(Math.sqrt(p.maximum * p.minimum) / Math.abs(p.minimum))).toExponential()}}{{??}}0.0{{?}}
	},
{{~}}
};

static size_t parameterOutDataToInfoIndex[DATA_PRODUCT_PARAMETERS_OUT_N] = {
	{{=it.product.parameters.filter(x => !x.isLatency && x.direction == "output").map(x => x.paramInfoIndex).join(", ")}}
};
#  endif

static size_t parameterInfoToDataIndex[DATA_PRODUCT_PARAMETERS_N] = {
	{{=it.product.parameters.filter(x => !x.isLatency).map(x => x.paramDataIndex).join(", ")}}
};

# endif

#endif

{{?it.product.parameters.find(x => x.isLatency)}}
#define DATA_PARAM_LATENCY_INDEX		{{=it.product.parameters.find(x => x.isLatency).paramIndex}}
{{?}}

{{?it.product.ui}}
#define DATA_UI
#define DATA_UI_USER_RESIZABLE			{{=it.product.ui.userResizable ? 1 : 0}}
{{?}}

/*
 * Parameter indices/ids:
 *
 * parameterInfo.id: hash of parameter id (+ extra sometimes), univocally identifies parameter across plugin versions (a la lv2:symbol)
 * parameterGetIndexById(): returns parameterInfo array index based on id (parameterInfo.id)
 * parameterInfoToDataIndex: maps parameterInfo array indices to parameter(In/Out)Data indices
 * parameterOutDataToInfoIndex: maps parameterOutData indices to parameterInfo array indices
 * parameter(In/Out)Data.index/p.paramIndex: Tibia parameter index, as used in plugin.h
 * latency out parameter is never added to parameterInfo and parameter(In/Out)Data (specially handled)
 */

{{?it.product.state && it.product.state.dspCustom}}
#define DATA_STATE_DSP_CUSTOM
{{?}}
