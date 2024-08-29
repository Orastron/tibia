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

#include <stdlib.h>
#include <stdint.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include "vst3_c_api.h"
#pragma GCC diagnostic pop

#include "data.h"
#include "plugin_api.h"
#include "plugin.h"
#ifdef DATA_UI
# include "plugin_ui.h"
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#if defined(_WIN32) || defined(__CYGWIN__)
# include <windows.h>
#else
# include <dlfcn.h>
#endif
#ifdef __APPLE__
# include <CoreFoundation/CoreFoundation.h>
#endif

#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#include <pmmintrin.h>
#endif

#ifdef PARAM_OUT_CPU_INDEX
# include "fatica.h"
#endif

// COM in C doc:
//   https://github.com/rubberduck-vba/Rubberduck/wiki/COM-in-plain-C
//   https://devblogs.microsoft.com/oldnewthing/20040205-00/?p=40733

//#define TIBIA_TRACE
#ifdef TIBIA_TRACE
# define TRACE(...)	printf(__VA_ARGS__); fflush(stdout);
#else
# define TRACE(...)	/* do nothing */
#endif

#ifdef DATA_UI
# ifdef __linux__
// Why generate the C interface when you can just not give a fuck? Thank you Steinberg!

typedef struct Steinberg_ITimerHandlerVtbl
{
	/* methods derived from "Steinberg_FUnknown": */
	Steinberg_tresult (SMTG_STDMETHODCALLTYPE* queryInterface) (void* thisInterface, const Steinberg_TUID iid, void** obj);
	Steinberg_uint32  (SMTG_STDMETHODCALLTYPE* addRef) (void* thisInterface);
	Steinberg_uint32  (SMTG_STDMETHODCALLTYPE* release) (void* thisInterface);

	/* methods derived from "Steinberg_ITimerHandler": */
	void (SMTG_STDMETHODCALLTYPE* onTimer) (void* thisInterface);
} Steinberg_ITimerHandlerVtbl;

typedef struct Steinberg_ITimerHandler
{
    struct Steinberg_ITimerHandlerVtbl* lpVtbl;
} Steinberg_ITimerHandler;

static const Steinberg_TUID Steinberg_ITimerHandler_iid = SMTG_INLINE_UID (0x10BDD94F, 0x41424774, 0x821FAD8F, 0xECA72CA9);

typedef struct Steinberg_IEventHandlerVtbl
{
	/* methods derived from "Steinberg_FUnknown": */
	Steinberg_tresult (SMTG_STDMETHODCALLTYPE* queryInterface) (void* thisInterface, const Steinberg_TUID iid, void** obj);
	Steinberg_uint32  (SMTG_STDMETHODCALLTYPE* addRef) (void* thisInterface);
	Steinberg_uint32  (SMTG_STDMETHODCALLTYPE* release) (void* thisInterface);

	/* methods derived from "Steinberg_IEventHandler": */
	void (SMTG_STDMETHODCALLTYPE* onFDIsSet) (void* thisInterface, int fd);
} Steinberg_IEventHandlerVtbl;

typedef struct Steinberg_IEventHandler
{
    struct Steinberg_IEventHandlerVtbl* lpVtbl;
} Steinberg_IEventHandler;

// not used
//static const Steinberg_TUID Steinberg_IEventHandler_iid = SMTG_INLINE_UID (0x561E65C9, 0x13A0496F, 0x813A2C35, 0x654D7983);

typedef struct Steinberg_IRunLoopVtbl
{
	/* methods derived from "Steinberg_FUnknown": */
	Steinberg_tresult (SMTG_STDMETHODCALLTYPE* queryInterface) (void* thisInterface, const Steinberg_TUID iid, void** obj);
	Steinberg_uint32  (SMTG_STDMETHODCALLTYPE* addRef) (void* thisInterface);
	Steinberg_uint32  (SMTG_STDMETHODCALLTYPE* release) (void* thisInterface);

	/* methods derived from "Steinberg_IRunLoop": */
	Steinberg_tresult (SMTG_STDMETHODCALLTYPE* registerEventHandler) (void* thisInterface, struct Steinberg_IEventHandler* handler, int fd);
	Steinberg_tresult (SMTG_STDMETHODCALLTYPE* unregisterEventHandler) (void* thisInterface, struct Steinberg_IEventHandler* handler);
	Steinberg_tresult (SMTG_STDMETHODCALLTYPE* registerTimer) (void* thisInterface, struct Steinberg_ITimerHandler* handler, uint64_t milliseconds);
	Steinberg_tresult (SMTG_STDMETHODCALLTYPE* unregisterTimer) (void* thisInterface, struct Steinberg_ITimerHandler* handler);
} Steinberg_IRunLoopVtbl;

typedef struct Steinberg_IRunLoop
{
    struct Steinberg_IRunLoopVtbl* lpVtbl;
} Steinberg_IRunLoop;

static const Steinberg_TUID Steinberg_IRunLoop_iid = SMTG_INLINE_UID (0x18C35366, 0x97764F1A, 0x9C5B8385, 0x7A871389);
# endif
#endif

#if defined(__linux__) || defined(__APPLE__)
static char *x_asprintf(const char * restrict format, ...) {
	va_list args, tmp;
	va_start(args, format);
	va_copy(tmp, args);
	int len = vsnprintf(NULL, 0, format, tmp);
	va_end(tmp);
	char *s = malloc(len + 1);
	if (s != NULL)
		vsprintf(s, format, args);
	va_end(args);
	return s;
}
#endif

static char *bindir;
static char *datadir;

static const char * get_bindir_cb(void *handle) {
	(void)handle;
	return bindir;
}

static const char * get_datadir_cb(void *handle) {
	(void)handle;
	return datadir;
}

static double clamp(double x, double m, double M) {
	return x < m ? m : (x > M ? M : x);
}

static int parameterGetIndexById(Steinberg_Vst_ParamID id) {
	for (int i = 0; i < DATA_PRODUCT_PARAMETERS_N + 3 * DATA_PRODUCT_BUSES_MIDI_INPUT_N; i++)
		if (parameterInfo[i].id == id)
			return i;
	return -1;
}

static double parameterMap(int i, double v) {
	return parameterData[i].flags & DATA_PARAM_MAP_LOG ? parameterData[i].min * exp(parameterData[i].mapK * v) : parameterData[i].min + (parameterData[i].max - parameterData[i].min) * v;
}

static double parameterUnmap(int i, double v) {
	return parameterData[i].flags & DATA_PARAM_MAP_LOG ? log(v / parameterData[i].min) / parameterData[i].mapK : (v - parameterData[i].min) / (parameterData[i].max - parameterData[i].min);
}

static double parameterAdjust(int i, double v) {
	v = parameterData[i].flags & (DATA_PARAM_BYPASS | DATA_PARAM_TOGGLED) ? (v >= 0.5 ? 1.0 : 0.0)
		: (parameterData[i].flags & DATA_PARAM_INTEGER ? (int32_t)(v + (v >= 0.0 ? 0.5 : -0.5)) : v);
	return clamp(v, parameterData[i].min, parameterData[i].max);
}

typedef struct pluginInstance {
	Steinberg_Vst_IComponentVtbl                  *vtblIComponent;
	Steinberg_Vst_IAudioProcessorVtbl             *vtblIAudioProcessor;
	Steinberg_Vst_IProcessContextRequirementsVtbl *vtblIProcessContextRequirements;
	Steinberg_uint32                               refs;
	Steinberg_FUnknown                            *context;
	plugin                                         p;
	float                                          sampleRate;
#if DATA_PRODUCT_PARAMETERS_N > 0
	float                                          parameters[DATA_PRODUCT_PARAMETERS_N];
#endif
#if DATA_PRODUCT_CHANNELS_AUDIO_INPUT_N > 0
	const float                                   *inputs[DATA_PRODUCT_CHANNELS_AUDIO_INPUT_N];
#endif
#if DATA_PRODUCT_CHANNELS_AUDIO_OUTPUT_N > 0
	float                                         *outputs[DATA_PRODUCT_CHANNELS_AUDIO_OUTPUT_N];
#endif
#if DATA_PRODUCT_BUSES_AUDIO_INPUT_N + DATA_PRODUCT_BUSES_AUDIO_OUTPUT_N + DATA_PRODUCT_BUSES_MIDI_INPUT_N + DATA_PRODUCT_BUSES_MIDI_OUTPUT_N > 0
	// see https://github.com/steinbergmedia/vst3sdk/issues/128
	char						neededBusesActive;
#endif
#if DATA_PRODUCT_BUSES_AUDIO_INPUT_N > 0
	char                                           inputsActive[DATA_PRODUCT_BUSES_AUDIO_INPUT_N];
#endif
#if DATA_PRODUCT_BUSES_AUDIO_OUTPUT_N > 0
	char                                           outputsActive[DATA_PRODUCT_BUSES_AUDIO_OUTPUT_N];
#endif
#if DATA_PRODUCT_BUSES_MIDI_INPUT_N > 0
	char                                           midiInputsActive[DATA_PRODUCT_BUSES_MIDI_INPUT_N];
#endif
#if DATA_PRODUCT_BUSES_MIDI_OUTPUT_N > 0
	char                                           midiOutputsActive[DATA_PRODUCT_BUSES_MIDI_OUTPUT_N];
#endif
	void                                          *mem;
#ifdef PARAM_OUT_CPU_INDEX
	float                                          cpu_meter;
#endif
} pluginInstance;

static Steinberg_Vst_IComponentVtbl pluginVtblIComponent;
static Steinberg_Vst_IAudioProcessorVtbl pluginVtblIAudioProcessor;

static Steinberg_tresult pluginQueryInterface(pluginInstance *p, const Steinberg_TUID iid, void ** obj) {
	// This seems to violate the way multiple inheritance should work in COM, but hosts like it, so what do I know...
	size_t offset;
	if (memcmp(iid, Steinberg_FUnknown_iid, sizeof(Steinberg_TUID)) == 0
	    || memcmp(iid, Steinberg_IPluginBase_iid, sizeof(Steinberg_TUID)) == 0
	    || memcmp(iid, Steinberg_Vst_IComponent_iid, sizeof(Steinberg_TUID)) == 0)
		offset = offsetof(pluginInstance, vtblIComponent);
	else if (memcmp(iid, Steinberg_Vst_IAudioProcessor_iid, sizeof(Steinberg_TUID)) == 0)
		offset = offsetof(pluginInstance, vtblIAudioProcessor);
	else if (memcmp(iid, Steinberg_Vst_IProcessContextRequirements_iid, sizeof(Steinberg_TUID)) == 0)
		offset = offsetof(pluginInstance, vtblIProcessContextRequirements);
	else {
		TRACE(" not supported\n");
		for (int i = 0; i < 16; i++)
			TRACE(" %x", iid[i]);
		TRACE("\n");
		*obj = NULL;
		return Steinberg_kNoInterface;
	}
	*obj = (void *)((char *)p + offset);
	p->refs++;
	return Steinberg_kResultOk;
}

static Steinberg_uint32 pluginAddRef(pluginInstance *p) {
	p->refs++;
	return p->refs;
}

static Steinberg_uint32 pluginRelease(pluginInstance *p) {
	p->refs--;
	if (p->refs == 0) {
		TRACE(" free %p\n", (void *)p);
		free(p);
		return 0;
	}
	return p->refs;
}

static Steinberg_tresult pluginIComponentQueryInterface(void *thisInterface, const Steinberg_TUID iid, void ** obj) {
	TRACE("plugin IComponent queryInterface %p\n", thisInterface);
	return pluginQueryInterface((pluginInstance *)((char *)thisInterface - offsetof(pluginInstance, vtblIComponent)), iid, obj);
}

static Steinberg_uint32 pluginIComponentAddRef(void *thisInterface) {
	TRACE("plugin IComponent addRef %p\n", thisInterface);
	return pluginAddRef((pluginInstance *)((char *)thisInterface - offsetof(pluginInstance, vtblIComponent)));
}

static Steinberg_uint32 pluginIComponentRelease(void *thisInterface) {
	TRACE("plugin IComponent release %p\n", thisInterface);
	return pluginRelease((pluginInstance *)((char *)thisInterface - offsetof(pluginInstance, vtblIComponent)));
}

static Steinberg_tresult pluginInitialize(void *thisInterface, struct Steinberg_FUnknown *context) {
	TRACE("plugin initialize\n");

	pluginInstance *p = (pluginInstance *)((char *)thisInterface - offsetof(pluginInstance, vtblIComponent));
	if (p->context != NULL)
		return Steinberg_kResultFalse;
	p->context = context;

	plugin_callbacks cbs = {
		/* .handle      = */ (void *)p,
		/* .format      = */ "vst3",
		/* .get_bindir  = */ get_bindir_cb,
		/* .get_datadir = */ get_datadir_cb
	};
	plugin_init(&p->p, &cbs);
#if DATA_PRODUCT_PARAMETERS_N > 0
	for (size_t i = 0; i < DATA_PRODUCT_PARAMETERS_N; i++) {
		p->parameters[i] = parameterData[i].def;
		if (!(parameterInfo[i].flags & Steinberg_Vst_ParameterInfo_ParameterFlags_kIsReadOnly))
			plugin_set_parameter(&p->p, parameterData[i].index, parameterData[i].def);
	}
#endif
#if DATA_PRODUCT_BUSES_AUDIO_INPUT_N + DATA_PRODUCT_BUSES_AUDIO_OUTPUT_N + DATA_PRODUCT_BUSES_MIDI_INPUT_N + DATA_PRODUCT_BUSES_MIDI_OUTPUT_N > 0
	p->neededBusesActive = 0;
# if DATA_PRODUCT_BUSES_AUDIO_INPUT_N > 0
	for (size_t i = 0; i < DATA_PRODUCT_BUSES_AUDIO_INPUT_N; i++)
		p->inputsActive[i] = 0;
# endif
# if DATA_PRODUCT_BUSES_AUDIO_OUTPUT_N > 0
	for (size_t i = 0; i < DATA_PRODUCT_BUSES_AUDIO_OUTPUT_N; i++)
		p->outputsActive[i] = 0;
# endif
# if DATA_PRODUCT_BUSES_MIDI_INPUT_N > 0
	for (size_t i = 0; i < DATA_PRODUCT_BUSES_MIDI_INPUT_N; i++)
		p->midiInputsActive[i] = 0;
# endif
# if DATA_PRODUCT_BUSES_MIDI_OUTPUT_N > 0
	for (size_t i = 0; i < DATA_PRODUCT_BUSES_MIDI_OUTPUT_N; i++)
		p->midiOutputsActive[i] = 0;
# endif
#endif
	p->mem = NULL;
#ifdef PARAM_OUT_CPU_INDEX
	p->cpu_meter = 0.f;
#endif
	return Steinberg_kResultOk;
}

static Steinberg_tresult pluginTerminate(void *thisInterface) {
	TRACE("plugin terminate\n");
	pluginInstance *p = (pluginInstance *)((char *)thisInterface - offsetof(pluginInstance, vtblIComponent));
	p->context = NULL;
	plugin_fini(&p->p);
	if (p->mem)
		free(p->mem);
	return Steinberg_kResultOk;
}

static Steinberg_tresult pluginGetControllerClassId(void *thisInterface, Steinberg_TUID classId) {
	(void)thisInterface;

	TRACE("plugin get controller class id %p %p\n", thisInterface, classId);
	if (classId != NULL) {
		memcpy(classId, dataControllerCID, sizeof(Steinberg_TUID));
		return Steinberg_kResultTrue;
	}
	return Steinberg_kResultFalse;
}

static Steinberg_tresult pluginSetIoMode(void *thisInterface, Steinberg_Vst_IoMode mode) {
	(void)thisInterface;
	(void)mode;

	TRACE("plugin set io mode\n");
	return Steinberg_kNotImplemented;
}

static Steinberg_int32 pluginGetBusCount(void *thisInterface, Steinberg_Vst_MediaType type, Steinberg_Vst_BusDirection dir) {
	(void)thisInterface;

	TRACE("plugin get bus count\n");
	if (type == Steinberg_Vst_MediaTypes_kAudio) {
		if (dir == Steinberg_Vst_BusDirections_kInput)
			return DATA_PRODUCT_BUSES_AUDIO_INPUT_N;
		else if (dir == Steinberg_Vst_BusDirections_kOutput)
			return DATA_PRODUCT_BUSES_AUDIO_OUTPUT_N;
	} else if (type == Steinberg_Vst_MediaTypes_kEvent) {
		if (dir == Steinberg_Vst_BusDirections_kInput)
			return DATA_PRODUCT_BUSES_MIDI_INPUT_N;
		else if (dir == Steinberg_Vst_BusDirections_kOutput)
			return DATA_PRODUCT_BUSES_MIDI_OUTPUT_N;
	}
	return 0;
}

static Steinberg_tresult pluginGetBusInfo(void* thisInterface, Steinberg_Vst_MediaType type, Steinberg_Vst_BusDirection dir, Steinberg_int32 index, struct Steinberg_Vst_BusInfo* bus) {
	(void)thisInterface;

	TRACE("plugin get bus info\n");
	if (index < 0)
		return Steinberg_kInvalidArgument;
	if (type == Steinberg_Vst_MediaTypes_kAudio) {
		if (dir == Steinberg_Vst_BusDirections_kInput) {
#if DATA_PRODUCT_BUSES_AUDIO_INPUT_N > 0
			if (index >= DATA_PRODUCT_BUSES_AUDIO_INPUT_N)
				return Steinberg_kInvalidArgument;
			*bus = busInfoAudioInput[index];
			return Steinberg_kResultTrue;
#endif
		} else if (dir == Steinberg_Vst_BusDirections_kOutput) {
#if DATA_PRODUCT_BUSES_AUDIO_OUTPUT_N > 0
			if (index >= DATA_PRODUCT_BUSES_AUDIO_OUTPUT_N)
				return Steinberg_kInvalidArgument;
			*bus = busInfoAudioOutput[index];
			return Steinberg_kResultTrue;
#endif
		}
	} else if (type == Steinberg_Vst_MediaTypes_kEvent) {
		if (dir == Steinberg_Vst_BusDirections_kInput) {
#if DATA_PRODUCT_BUSES_MIDI_INPUT_N > 0
			if (index >= DATA_PRODUCT_BUSES_MIDI_INPUT_N)
				return Steinberg_kInvalidArgument;
			*bus = busInfoMidiInput[index];
			return Steinberg_kResultTrue;
#endif
		} else if (dir == Steinberg_Vst_BusDirections_kOutput) {
#if DATA_PRODUCT_BUSES_MIDI_OUTPUT_N > 0
			if (index >= DATA_PRODUCT_BUSES_MIDI_OUTPUT_N)
				return Steinberg_kInvalidArgument;
			*bus = busInfoMidiOutput[index];
			return Steinberg_kResultTrue;
#endif
		}
	}
	return Steinberg_kInvalidArgument;
}

static Steinberg_tresult pluginGetRoutingInfo(void* thisInterface, struct Steinberg_Vst_RoutingInfo* inInfo, struct Steinberg_Vst_RoutingInfo* outInfo) {
	(void)thisInterface;
	(void)inInfo;
	(void)outInfo;

	TRACE("plugin get routing info\n");
	return Steinberg_kNotImplemented;
}

static Steinberg_tresult pluginActivateBus(void* thisInterface, Steinberg_Vst_MediaType type, Steinberg_Vst_BusDirection dir, Steinberg_int32 index, Steinberg_TBool state) {
	TRACE("plugin activate bus\n");
	if (index < 0)
		return Steinberg_kInvalidArgument;
	pluginInstance *p = (pluginInstance *)((char *)thisInterface - offsetof(pluginInstance, vtblIComponent));
	if (type == Steinberg_Vst_MediaTypes_kAudio) {
		if (dir == Steinberg_Vst_BusDirections_kInput) {
#if DATA_PRODUCT_BUSES_AUDIO_INPUT_N > 0
			if (index >= DATA_PRODUCT_BUSES_AUDIO_INPUT_N)
				return Steinberg_kInvalidArgument;
			p->inputsActive[index] = state;
			return Steinberg_kResultTrue;
#endif
		} else if (dir == Steinberg_Vst_BusDirections_kOutput) {
#if DATA_PRODUCT_BUSES_AUDIO_OUTPUT_N > 0
			if (index >= DATA_PRODUCT_BUSES_AUDIO_OUTPUT_N)
				return Steinberg_kInvalidArgument;
			p->outputsActive[index] = state;
			return Steinberg_kResultTrue;
#endif
		}
	} else if (type == Steinberg_Vst_MediaTypes_kEvent) {
		if (dir == Steinberg_Vst_BusDirections_kInput) {
#if DATA_PRODUCT_BUSES_MIDI_INPUT_N > 0
			if (index >= DATA_PRODUCT_BUSES_MIDI_INPUT_N)
				return Steinberg_kInvalidArgument;
			p->midiInputsActive[index] = state;
			return Steinberg_kResultTrue;
#endif
		} else if (dir == Steinberg_Vst_BusDirections_kOutput) {
#if DATA_PRODUCT_BUSES_MIDI_OUTPUT_N > 0
			if (index >= DATA_PRODUCT_BUSES_MIDI_OUTPUT_N)
				return Steinberg_kInvalidArgument;
			p->midiOutputsActive[index] = state;
			return Steinberg_kResultTrue;
#endif
		}
	}
	return Steinberg_kInvalidArgument;
}

static Steinberg_tresult pluginSetActive(void* thisInterface, Steinberg_TBool state) {
	TRACE("plugin set active\n");
	pluginInstance *p = (pluginInstance *)((char *)thisInterface - offsetof(pluginInstance, vtblIComponent));
	if (p->mem != NULL) {
		free(p->mem);
		p->mem = NULL;
	}
	if (state) {
#if DATA_PRODUCT_BUSES_AUDIO_INPUT_N + DATA_PRODUCT_BUSES_AUDIO_OUTPUT_N + DATA_PRODUCT_BUSES_MIDI_INPUT_N + DATA_PRODUCT_BUSES_MIDI_OUTPUT_N > 0
		p->neededBusesActive = 1;
# if DATA_PRODUCT_BUSES_AUDIO_INPUT_N > 0
		for (size_t i = 0; p->neededBusesActive && i < DATA_PRODUCT_BUSES_AUDIO_INPUT_N; i++)
			if (!p->inputsActive[i] && (busInfoAudioInput[i].flags & Steinberg_Vst_BusInfo_BusFlags_kDefaultActive))
				p->neededBusesActive = 0;
# endif
# if DATA_PRODUCT_BUSES_AUDIO_OUTPUT_N > 0
		for (size_t i = 0; p->neededBusesActive && i < DATA_PRODUCT_BUSES_AUDIO_OUTPUT_N; i++)
			if (!p->outputsActive[i] && (busInfoAudioOutput[i].flags & Steinberg_Vst_BusInfo_BusFlags_kDefaultActive))
				p->neededBusesActive = 0;
# endif
# if DATA_PRODUCT_BUSES_MIDI_INPUT_N > 0
		for (size_t i = 0; p->neededBusesActive && i < DATA_PRODUCT_BUSES_MIDI_INPUT_N; i++)
			if (!p->midiInputsActive[i] && (busInfoMidiInput[i].flags & Steinberg_Vst_BusInfo_BusFlags_kDefaultActive))
				p->neededBusesActive = 0;
# endif
# if DATA_PRODUCT_BUSES_MIDI_OUTPUT_N > 0
		for (size_t i = 0; p->neededBusesActive && i < DATA_PRODUCT_BUSES_MIDI_OUTPUT_N; i++)
			if (!p->midiOutputsActive[i] && (busInfoMidiOutput[i].flags & Steinberg_Vst_BusInfo_BusFlags_kDefaultActive))
				p->neededBusesActive = 0;
# endif
#endif
		plugin_set_sample_rate(&p->p, p->sampleRate);
		size_t req = plugin_mem_req(&p->p);
		if (req != 0) {
			p->mem = malloc(req);
			if (p->mem == NULL)
				return Steinberg_kOutOfMemory;
			plugin_mem_set(&p->p, p->mem);
		}
		plugin_reset(&p->p);
	}
	return Steinberg_kResultOk;
}

// https://stackoverflow.com/questions/2100331/macro-definition-to-determine-big-endian-or-little-endian-machine
#define IS_BIG_ENDIAN (!(union { uint16_t u16; unsigned char c; }){ .u16 = 1 }.c)
// https://stackoverflow.com/questions/2182002/how-to-convert-big-endian-to-little-endian-in-c-without-using-library-functions
#define SWAP_UINT32(x) (((x) >> 24) | (((x) & 0x00FF0000) >> 8) | (((x) & 0x0000FF00) << 8) | ((x) << 24))

static Steinberg_tresult pluginSetState(void* thisInterface, struct Steinberg_IBStream* state) {
	TRACE("plugin set state\n");
	if (state == NULL)
		return Steinberg_kResultFalse;
#if DATA_PRODUCT_PARAMETERS_N > 0
	pluginInstance *p = (pluginInstance *)((char *)thisInterface - offsetof(pluginInstance, vtblIComponent));
	for (size_t i = 0; i < DATA_PRODUCT_PARAMETERS_N; i++) {
		if (parameterInfo[i].flags & Steinberg_Vst_ParameterInfo_ParameterFlags_kIsReadOnly)
			continue;
		union { float f; uint32_t u; } v;
		Steinberg_int32 n;
		state->lpVtbl->read(state, &v, 4, &n);
		if (n != 4)
			return Steinberg_kResultFalse;
		if (IS_BIG_ENDIAN)
			v.u = SWAP_UINT32(v.u);
		p->parameters[i] = parameterAdjust(i, v.f);
		plugin_set_parameter(&p->p, parameterData[i].index, p->parameters[i]);
	}
#endif
	return Steinberg_kResultOk;
}

static Steinberg_tresult pluginGetState(void* thisInterface, struct Steinberg_IBStream* state) {
	TRACE("plugin get state\n");
	if (state == NULL)
		return Steinberg_kResultFalse;
#if DATA_PRODUCT_PARAMETERS_N > 0
	pluginInstance *p = (pluginInstance *)((char *)thisInterface - offsetof(pluginInstance, vtblIComponent));
	for (size_t i = 0; i < DATA_PRODUCT_PARAMETERS_N; i++) {
		if (parameterInfo[i].flags & Steinberg_Vst_ParameterInfo_ParameterFlags_kIsReadOnly)
			continue;
		union { float f; uint32_t u; } v;
		v.f = p->parameters[i];
		if (IS_BIG_ENDIAN)
			v.u = SWAP_UINT32(v.u);
		Steinberg_int32 n;
		state->lpVtbl->write(state, &v, 4, &n);
		if (n != 4)
			return Steinberg_kResultFalse;
	}
#endif
	return Steinberg_kResultOk;
}

static Steinberg_Vst_IComponentVtbl pluginVtblIComponent = {
	/* FUnknown */
	/* .queryInterface       = */ pluginIComponentQueryInterface,
	/* .addRef               = */ pluginIComponentAddRef,
	/* .release              = */ pluginIComponentRelease,

	/* IPluginBase */
	/* .initialize           = */ pluginInitialize,
	/* .terminate            = */ pluginTerminate,

	/* IComponent */
	/* .getControllerClassId = */ pluginGetControllerClassId,
	/* .setIoMode            = */ pluginSetIoMode,
	/* .getBusCount          = */ pluginGetBusCount,
	/* .getBusInfo           = */ pluginGetBusInfo,
	/* .getRoutingInfo       = */ pluginGetRoutingInfo,
	/* .activateBus          = */ pluginActivateBus,
	/* .setActive            = */ pluginSetActive,
	/* .setState             = */ pluginSetState,
	/* .getState             = */ pluginGetState
};

static Steinberg_tresult pluginIAudioProcessorQueryInterface(void *thisInterface, const Steinberg_TUID iid, void ** obj) {
	TRACE("plugin IAudioProcessor queryInterface %p\n", thisInterface);
	return pluginQueryInterface((pluginInstance *)((char *)thisInterface - offsetof(pluginInstance, vtblIAudioProcessor)), iid, obj);
}

static Steinberg_uint32 pluginIAudioProcessorAddRef(void *thisInterface) {
	TRACE("plugin IAudioProcessor addRef %p\n", thisInterface);
	return pluginAddRef((pluginInstance *)((char *)thisInterface - offsetof(pluginInstance, vtblIAudioProcessor)));
}

static Steinberg_uint32 pluginIAudioProcessorRelease(void *thisInterface) {
	TRACE("plugin IAudioProcessor release %p\n", thisInterface);
	return pluginRelease((pluginInstance *)((char *)thisInterface - offsetof(pluginInstance, vtblIAudioProcessor)));
}

static Steinberg_tresult pluginSetBusArrangements(void* thisInterface, Steinberg_Vst_SpeakerArrangement* inputs, Steinberg_int32 numIns, Steinberg_Vst_SpeakerArrangement* outputs, Steinberg_int32 numOuts) {
	(void)thisInterface;

	TRACE("plugin IAudioProcessor set bus arrangements\n");
	if (numIns < 0 || numOuts < 0)
		return Steinberg_kInvalidArgument;
	if (numIns != DATA_PRODUCT_BUSES_AUDIO_INPUT_N || numOuts != DATA_PRODUCT_BUSES_AUDIO_OUTPUT_N)
		return Steinberg_kResultFalse;

#if DATA_PRODUCT_BUSES_AUDIO_INPUT_N > 0
	for (Steinberg_int32 i = 0; i < numIns; i++)
		if ((busInfoAudioInput[i].channelCount == 1 && inputs[i] != Steinberg_Vst_SpeakerArr_kMono)
		    || (busInfoAudioInput[i].channelCount == 2 && inputs[i] != Steinberg_Vst_SpeakerArr_kStereo))
			return Steinberg_kResultFalse;
#else
	(void)inputs;
#endif

#if DATA_PRODUCT_BUSES_AUDIO_OUTPUT_N > 0
	for (Steinberg_int32 i = 0; i < numOuts; i++)
		if ((busInfoAudioOutput[i].channelCount == 1 && outputs[i] != Steinberg_Vst_SpeakerArr_kMono)
		    || (busInfoAudioOutput[i].channelCount == 2 && outputs[i] != Steinberg_Vst_SpeakerArr_kStereo))
			return Steinberg_kResultFalse;
#else
	(void)outputs;
#endif

	return Steinberg_kResultTrue;
}

static Steinberg_tresult pluginGetBusArrangement(void* thisInterface, Steinberg_Vst_BusDirection dir, Steinberg_int32 index, Steinberg_Vst_SpeakerArrangement* arr) {
	(void)thisInterface;

	TRACE("plugin IAudioProcessor get bus arrangement\n");

#if DATA_PRODUCT_BUSES_AUDIO_INPUT_N > 0
	if (dir == Steinberg_Vst_BusDirections_kInput && index >= 0 && index < DATA_PRODUCT_BUSES_AUDIO_INPUT_N) {
		*arr = busInfoAudioInput[index].channelCount == 1 ? Steinberg_Vst_SpeakerArr_kMono : Steinberg_Vst_SpeakerArr_kStereo;
		return Steinberg_kResultTrue;
	}
#endif

#if DATA_PRODUCT_BUSES_AUDIO_OUTPUT_N > 0
	if (dir == Steinberg_Vst_BusDirections_kOutput && index >= 0 && index < DATA_PRODUCT_BUSES_AUDIO_OUTPUT_N) {
		*arr = busInfoAudioOutput[index].channelCount == 1 ? Steinberg_Vst_SpeakerArr_kMono : Steinberg_Vst_SpeakerArr_kStereo;
		return Steinberg_kResultTrue;
	}
#endif

	return Steinberg_kInvalidArgument;
}

static Steinberg_tresult pluginCanProcessSampleSize(void* thisInterface, Steinberg_int32 symbolicSampleSize) {
	(void)thisInterface;

	TRACE("plugin IAudioProcessor can process sample size\n");
	return symbolicSampleSize == Steinberg_Vst_SymbolicSampleSizes_kSample32 ? Steinberg_kResultOk : Steinberg_kNotImplemented;
}

static Steinberg_uint32 pluginGetLatencySamples(void* thisInterface) {
	(void)thisInterface;

	TRACE("plugin IAudioProcessor get latency samples\n");
	//TBD
	return 0;
}

static Steinberg_tresult pluginSetupProcessing(void* thisInterface, struct Steinberg_Vst_ProcessSetup* setup) {
	TRACE("plugin IAudioProcessor setup processing\n");
	pluginInstance *p = (pluginInstance *)((char *)thisInterface - offsetof(pluginInstance, vtblIAudioProcessor));
	p->sampleRate = (float)setup->sampleRate;
	return Steinberg_kResultOk;
}

static Steinberg_tresult pluginSetProcessing(void* thisInterface, Steinberg_TBool state) {
	(void)thisInterface;
	(void)state;

	TRACE("plugin IAudioProcessor set processing\n");
	return Steinberg_kNotImplemented;
}

static void processParams(pluginInstance *p, struct Steinberg_Vst_ProcessData *data, char before) {
#if DATA_PRODUCT_PARAMETERS_N + DATA_PRODUCT_BUSES_MIDI_INPUT_N > 0
	if (data->inputParameterChanges == NULL)
		return;
	Steinberg_int32 n = data->inputParameterChanges->lpVtbl->getParameterCount(data->inputParameterChanges);
	for (Steinberg_int32 i = 0; i < n; i++) {
		struct Steinberg_Vst_IParamValueQueue *q = data->inputParameterChanges->lpVtbl->getParameterData(data->inputParameterChanges, i);
		if (q == NULL)
			continue;
		Steinberg_int32 c = q->lpVtbl->getPointCount(q);
		if (c <= 0)
			continue;
		Steinberg_int32 o;
		Steinberg_Vst_ParamValue v;
		if (q->lpVtbl->getPoint(q, before ? 0 : c - 1, &o, &v) != Steinberg_kResultTrue)
			continue;
		if (before ? o != 0 : o <= 0)
			continue;
		Steinberg_Vst_ParamID id = q->lpVtbl->getParameterId(q);
		int pi = parameterGetIndexById(id);
# if DATA_PRODUCT_BUSES_MIDI_INPUT_N > 0
		if (pi >= DATA_PRODUCT_PARAMETERS_N) {
			size_t j = pi - DATA_PRODUCT_PARAMETERS_N;
			size_t index = j / 3;
			uint8_t data[3];
			switch (j & 0x3) {
			case 0:
				// channel pressure
				data[0] = 0xd0 /* | channel */;
				data[1] = (uint8_t)(127.0 * v);
				data[2] = 0;
				break;
			case 1:
			{
				// pitch bend change
				uint16_t x = (uint16_t)(16382.0 * v) + 1;
				data[0] = 0xe0 /* | channel */;
				data[1] = (x >> 7) & 0x7f;
				data[2] = x & 0x7f;
			}
				break;
			case 2:
				// mod wheel
				data[0] = 0xb0 /* | channel */;
				data[1] = 1;
				data[2] = (uint8_t)(127.0 * v);
				break;
			default:
				continue;
			}
			plugin_midi_msg_in(&p->p, midiInIndex[index], data);
			continue;
		}
# endif
		v = parameterAdjust(pi, parameterMap(pi, v));
		if (v != p->parameters[pi]) {
			p->parameters[pi] = v;
			plugin_set_parameter(&p->p, parameterData[pi].index, p->parameters[pi]);
		}
	}
#endif
}

static Steinberg_tresult pluginProcess(void* thisInterface, struct Steinberg_Vst_ProcessData* data) {
	TRACE("plugin IAudioProcessor process\n");

#ifdef PARAM_OUT_CPU_INDEX
	const unsigned long long processTimeStart = fatica_time_process();
#endif

#if defined(__aarch64__)
	uint64_t fpcr;
	__asm__ __volatile__ ("mrs %0, fpcr" : "=r"(fpcr));
	__asm__ __volatile__ ("msr fpcr, %0" :: "r"(fpcr | 0x1000000)); // enable FZ
#elif defined(__i386__) || defined(__x86_64__)
	const unsigned int flush_zero_mode = _MM_GET_FLUSH_ZERO_MODE();
	const unsigned int denormals_zero_mode = _MM_GET_DENORMALS_ZERO_MODE();

	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif

	pluginInstance *p = (pluginInstance *)((char *)thisInterface - offsetof(pluginInstance, vtblIAudioProcessor));

	processParams(p, data, 1); 

#if DATA_PRODUCT_BUSES_MIDI_INPUT_N > 0
	if (data->inputEvents != NULL) {
		Steinberg_int32 n = data->inputEvents->lpVtbl->getEventCount(data->inputEvents);
		for (Steinberg_int32 i = 0; i < n; i++) {
			struct Steinberg_Vst_Event ev;
			if (data->inputEvents->lpVtbl->getEvent(data->inputEvents, i, &ev) != Steinberg_kResultOk)
				continue;
			switch (ev.type) {
			case Steinberg_Vst_Event_EventTypes_kNoteOnEvent:
			{
				const uint8_t data[3] = { 0x90 | ev.Steinberg_Vst_Event_noteOn.channel, ev.Steinberg_Vst_Event_noteOn.pitch, (uint8_t)(127.f * ev.Steinberg_Vst_Event_noteOn.velocity) };
				plugin_midi_msg_in(&p->p, midiInIndex[ev.busIndex], data);
			}
				break;
			case Steinberg_Vst_Event_EventTypes_kNoteOffEvent:
			{
				const uint8_t data[3] = { 0x80 | ev.Steinberg_Vst_Event_noteOff.channel, ev.Steinberg_Vst_Event_noteOff.pitch, (uint8_t)(127.f * ev.Steinberg_Vst_Event_noteOff.velocity) };
				plugin_midi_msg_in(&p->p, midiInIndex[ev.busIndex], data);
			}
				break;
			case Steinberg_Vst_Event_EventTypes_kPolyPressureEvent:
			{
				const uint8_t data[3] = { 0xa0 | ev.Steinberg_Vst_Event_polyPressure.channel, ev.Steinberg_Vst_Event_polyPressure.pitch, (uint8_t)(127.f * ev.Steinberg_Vst_Event_polyPressure.pressure) };
				plugin_midi_msg_in(&p->p, midiInIndex[ev.busIndex], data);
			}
				break;
			}
		}
	}
#endif

#if DATA_PRODUCT_BUSES_AUDIO_INPUT_N + DATA_PRODUCT_BUSES_AUDIO_OUTPUT_N + DATA_PRODUCT_BUSES_MIDI_INPUT_N + DATA_PRODUCT_BUSES_MIDI_OUTPUT_N > 0
	if (p->neededBusesActive) {
# if DATA_PRODUCT_CHANNELS_AUDIO_INPUT_N > 0
		const float **inputs = p->inputs;
		Steinberg_int32 ki = 0;
		for (Steinberg_int32 i = 0; i < data->numInputs; i++)
			for (Steinberg_int32 j = 0; j < data->inputs[i].numChannels; j++, ki++)
				inputs[ki] = data->inputs[i].Steinberg_Vst_AudioBusBuffers_channelBuffers32[j];
# else
		const float **inputs = NULL;
# endif

# if DATA_PRODUCT_CHANNELS_AUDIO_OUTPUT_N > 0
		float **outputs = p->outputs;
		Steinberg_int32 ko = 0;
		for (Steinberg_int32 i = 0; i < data->numOutputs; i++)
			for (Steinberg_int32 j = 0; j < data->outputs[i].numChannels; j++, ko++)
				outputs[ko] = data->outputs[i].Steinberg_Vst_AudioBusBuffers_channelBuffers32[j];
# else
		float **outputs = NULL;
# endif

		plugin_process(&p->p, inputs, outputs, data->numSamples);
	} else {
# if DATA_PRODUCT_CHANNELS_AUDIO_OUTPUT_N > 0
		for (Steinberg_int32 i = 0; i < data->numOutputs; i++)
			for (Steinberg_int32 j = 0; j < data->outputs[i].numChannels; j++)
				memset(data->outputs[i].Steinberg_Vst_AudioBusBuffers_channelBuffers32[j], 0, data->numSamples * sizeof(float));
# endif
	}
#else
	plugin_process(&p->p, NULL, NULL, data->numSamples);
#endif

	processParams(p, data, 0); 

#if DATA_PRODUCT_PARAMETERS_N > 0
	for (Steinberg_int32 i = 0; i < DATA_PRODUCT_PARAMETERS_N; i++) {
		if (!(parameterInfo[i].flags & Steinberg_Vst_ParameterInfo_ParameterFlags_kIsReadOnly))
			continue;
		float v;
# ifdef PARAM_OUT_CPU_INDEX
		if (i == PARAM_OUT_CPU_INDEX)
			v = p->cpu_meter;
		else
# endif
		v = plugin_get_parameter(&p->p, parameterData[i].index);
		if (v == p->parameters[i])
			continue;
		p->parameters[i] = v;
		if (data->outputParameterChanges == NULL)
			continue;
		Steinberg_Vst_ParamID id = parameterInfo[i].id;
		Steinberg_int32 index;
		struct Steinberg_Vst_IParamValueQueue *q = data->outputParameterChanges->lpVtbl->addParameterData(data->outputParameterChanges, &id, &index);
		if (q == NULL)
			continue;
		q->lpVtbl->addPoint(q, data->numSamples - 1, parameterUnmap(i, v), &index);
	}
#endif

	// TBD: latency + IComponentHandler::restartComponent (kLatencyChanged), see https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Workflow+Diagrams/Get+Latency+Call+Sequence.html

#if defined(__aarch64__)
	__asm__ __volatile__ ("msr fpcr, %0" : : "r"(fpcr));
#elif defined(__i386__) || defined(__x86_64__)
	_MM_SET_FLUSH_ZERO_MODE(flush_zero_mode);
	_MM_SET_DENORMALS_ZERO_MODE(denormals_zero_mode);
#endif

#ifdef PARAM_OUT_CPU_INDEX
	const unsigned long long processTimeEnd = fatica_time_process();
	const unsigned long long processTime100n = processTimeEnd - processTimeStart;
	const double processTimeS = ((double) processTime100n) * 1.0e-7;
	p->cpu_meter = p->cpu_meter * 0.9f + ((float) (processTimeS * p->sampleRate)) * 0.1f;
#endif

	return Steinberg_kResultOk;
}

static Steinberg_uint32 pluginGetTailSamples(void* thisInterface) {
	(void)thisInterface;

	TRACE("plugin IAudioProcessor get tail samples\n");
	//TBD
	return 0;
}

static Steinberg_Vst_IAudioProcessorVtbl pluginVtblIAudioProcessor = {
	/* FUnknown */
	/* .queryInterface       = */ pluginIAudioProcessorQueryInterface,
	/* .addRef               = */ pluginIAudioProcessorAddRef,
	/* .release              = */ pluginIAudioProcessorRelease,

	/* IAudioProcessor */
	/* .setBusArrangements   = */ pluginSetBusArrangements,
	/*. getBusArrangement    = */ pluginGetBusArrangement,
	/* .canProcessSampleSize = */ pluginCanProcessSampleSize,
	/* .getLatencySamples    = */ pluginGetLatencySamples,
	/* .setupProcessing      = */ pluginSetupProcessing,
	/* .setProcessing        = */ pluginSetProcessing,
	/* .process              = */ pluginProcess,
	/* .getTailSamples       = */ pluginGetTailSamples
};

static Steinberg_tresult pluginIProcessContextRequirementsQueryInterface(void *thisInterface, const Steinberg_TUID iid, void ** obj) {
	TRACE("plugin IProcessContextRequirements queryInterface %p\n", thisInterface);
	return pluginQueryInterface((pluginInstance *)((char *)thisInterface - offsetof(pluginInstance, vtblIProcessContextRequirements)), iid, obj);
}

static Steinberg_uint32 pluginIProcessContextRequirementsAddRef(void *thisInterface) {
	TRACE("plugin IComponent addRef %p\n", thisInterface);
	return pluginAddRef((pluginInstance *)((char *)thisInterface - offsetof(pluginInstance, vtblIProcessContextRequirements)));
}

static Steinberg_uint32 pluginIProcessContextRequirementsRelease(void *thisInterface) {
	TRACE("plugin IComponent release %p\n", thisInterface);
	return pluginRelease((pluginInstance *)((char *)thisInterface - offsetof(pluginInstance, vtblIProcessContextRequirements)));
}

static Steinberg_uint32 pluginGetProcessContextRequirements(void* thisInterface) {
	(void)thisInterface;

	// TBD
	return 0; 
}

static Steinberg_Vst_IProcessContextRequirementsVtbl pluginVtblIProcessContextRequirements = {
	/* FUnknown */
	/* .queryInterface                = */ pluginIProcessContextRequirementsQueryInterface,
	/* .addRef                        = */ pluginIProcessContextRequirementsAddRef,
	/* .release                       = */ pluginIProcessContextRequirementsRelease,

	/* IProcessContextRequirements */
	/* .getProcessContextRequirements = */ pluginGetProcessContextRequirements
};

typedef struct plugView plugView;

typedef struct controller {
	Steinberg_Vst_IEditControllerVtbl      *vtblIEditController;
	Steinberg_Vst_IMidiMappingVtbl         *vtblIMidiMapping;
#ifdef DATA_UI
	//Steinberg_Vst_IConnectionPointVtbl *		vtblIConnectionPoint;
#endif
	Steinberg_uint32                        refs;
	Steinberg_FUnknown                     *context;
#if DATA_PRODUCT_PARAMETERS_N + DATA_PRODUCT_BUSES_MIDI_INPUT_N > 0
	double                                  parameters[DATA_PRODUCT_PARAMETERS_N + 3 * DATA_PRODUCT_BUSES_MIDI_INPUT_N];
#endif
	struct Steinberg_Vst_IComponentHandler *componentHandler;
#ifdef DATA_UI
	plugView                              **views;
	size_t                                  viewsCount;
#endif
} controller;

static Steinberg_Vst_IEditControllerVtbl controllerVtblIEditController;

#ifdef DATA_UI

# ifdef __linux__

#  include <X11/Xlib.h>

typedef struct {
	Steinberg_ITimerHandlerVtbl *vtblITimerHandler;
	Steinberg_uint32             refs;
	void                        *data;
	void                         (*cb)(void *data);
} timerHandler;

static Steinberg_tresult timerHandlerQueryInterface(void *thisInterface, const Steinberg_TUID iid, void ** obj) {
	TRACE("timerHandler queryInterface %p\n", thisInterface);
	// Same as above (pluginQueryInterface)
	size_t offset;
	if (memcmp(iid, Steinberg_FUnknown_iid, sizeof(Steinberg_TUID)) == 0
	    || memcmp(iid, Steinberg_ITimerHandler_iid, sizeof(Steinberg_TUID)) == 0)
		offset = offsetof(timerHandler, vtblITimerHandler);
	else {
		TRACE(" not supported\n");
		for (int i = 0; i < 16; i++)
			TRACE(" %x", iid[i]);
		TRACE("\n");
		*obj = NULL;
		return Steinberg_kNoInterface;
	}
	timerHandler *t = (timerHandler *)((char *)thisInterface - offsetof(timerHandler, vtblITimerHandler));
	*obj = (void *)((char *)t + offset);
	t->refs++;
	return Steinberg_kResultOk;
}

static Steinberg_uint32 timerHandlerAddRef(void *thisInterface) {
	TRACE("timerHandler addRef %p\n", thisInterface);
	timerHandler *t = (timerHandler *)((char *)thisInterface - offsetof(timerHandler, vtblITimerHandler));
	t->refs++;
	return t->refs;
}

static Steinberg_uint32 timerHandlerRelease(void *thisInterface) {
	TRACE("timerHandler release %p\n", thisInterface);
	timerHandler *t = (timerHandler *)((char *)thisInterface - offsetof(timerHandler, vtblITimerHandler));
	t->refs--;
	if (t->refs == 0) {
		TRACE(" free %p\n", (void *)t);
		free(t);
		return 0;
	}
	return t->refs;
}

static void timerHandlerOnTimer(void* thisInterface) {
	TRACE("timerHandler onTimer %p\n", thisInterface);
	timerHandler *t = (timerHandler *)((char *)thisInterface - offsetof(timerHandler, vtblITimerHandler));
	t->cb(t->data);
}

static Steinberg_ITimerHandlerVtbl timerHandlerVtblITimerHandler = {
	/* FUnknown */
	/* .queryInterface = */ timerHandlerQueryInterface,
	/* .addRef         = */ timerHandlerAddRef,
	/* .release        = */ timerHandlerRelease,

	/* ITimerHandler */
	/* .onTimer        = */ timerHandlerOnTimer
};

# elif defined(__APPLE__)

#  include <objc/objc.h>
#  include <objc/runtime.h>
#  include <objc/message.h>

# endif

typedef struct plugView {
	Steinberg_IPlugViewVtbl *vtblIPlugView;
	Steinberg_uint32         refs;
	Steinberg_IPlugFrame    *frame;
	plugin_ui               *ui;
	controller              *ctrl;
# ifdef __linux__
	Steinberg_IRunLoop      *runLoop;
	timerHandler             timer;
	Display                 *display;
# elif defined(__APPLE__)
	CFRunLoopTimerRef        timer;
# elif defined(_WIN32) || defined(__CYGWIN__)
	UINT_PTR                 timer;
# endif
} plugView;

static Steinberg_tresult plugViewRemoved(void* thisInterface);

static Steinberg_tresult plugViewQueryInterface(void *thisInterface, const Steinberg_TUID iid, void ** obj) {
	TRACE("plugView queryInterface %p\n", thisInterface);
	// Same as above (pluginQueryInterface)
	size_t offset;
	if (memcmp(iid, Steinberg_FUnknown_iid, sizeof(Steinberg_TUID)) == 0
	    || memcmp(iid, Steinberg_IPlugView_iid, sizeof(Steinberg_TUID)) == 0)
		offset = offsetof(plugView, vtblIPlugView);
	else {
		TRACE(" not supported\n");
		for (int i = 0; i < 16; i++)
			TRACE(" %x", iid[i]);
		TRACE("\n");
		*obj = NULL;
		return Steinberg_kNoInterface;
	}
	plugView *v = (plugView *)((char *)thisInterface - offsetof(plugView, vtblIPlugView));
	*obj = (void *)((char *)v + offset);
	v->refs++;
	return Steinberg_kResultOk;
}

static Steinberg_uint32 plugViewAddRef(void *thisInterface) {
	TRACE("plugView addRef %p\n", thisInterface);
	plugView *v = (plugView *)((char *)thisInterface - offsetof(plugView, vtblIPlugView));
	v->refs++;
	return v->refs;
}

static Steinberg_uint32 plugViewRelease(void *thisInterface) {
	TRACE("plugView IEditController release %p\n", thisInterface);
	plugView *v = (plugView *)((char *)thisInterface - offsetof(plugView, vtblIPlugView));
	v->refs--;
	if (v->refs == 0) {
		TRACE(" free %p\n", (void *)v);
		if (v->ui)
			plugViewRemoved(thisInterface);
		for (size_t i = 0; i < v->ctrl->viewsCount; i++)
			if (v->ctrl->views[i] == v) {
				v->ctrl->views[i] = NULL;
				break;
			}
		free(v);
		return 0;
	}
	return v->refs;
}

static Steinberg_tresult plugViewIsPlatformTypeSupported(void* thisInterface, Steinberg_FIDString type) {
	(void)thisInterface;

	TRACE("plugView isPlatformTypeSupported %p %s\n", thisInterface, type);
# if defined(_WIN32) || defined(__CYGWIN__)
	return strcmp(type, "HWND") ? Steinberg_kResultFalse : Steinberg_kResultTrue;
# elif defined(__APPLE__) && defined(__MACH__)
	return strcmp(type, "NSView") ? Steinberg_kResultFalse : Steinberg_kResultTrue;
# elif defined(__linux__)
	return strcmp(type, "X11EmbedWindowID") ? Steinberg_kResultFalse : Steinberg_kResultTrue;
# else
	(void)type;

	return Steinberg_kResultFalse;
# endif
}

# if DATA_PRODUCT_PARAMETERS_N > 0
static void plugViewUpdateParameter(plugView *view, size_t index) {
	if (view->ui)
		plugin_ui_set_parameter(view->ui, parameterData[index].index, view->ctrl->parameters[index]);
}

static void plugViewUpdateAllParameters(plugView *view) {
	if (view->ui == NULL)
		return;
	for (size_t i = 0; i < DATA_PRODUCT_PARAMETERS_N; i++)
		plugin_ui_set_parameter(view->ui, parameterData[i].index, view->ctrl->parameters[i]);
}

static void plugViewSetParameterBeginCb(void *handle, size_t index) {
	TRACE("set parameter begin cb\n");

#  ifdef DATA_PARAM_LATENCY_INDEX
	if (index == DATA_PARAM_LATENCY_INDEX)
		return;
	index = index >= DATA_PARAM_LATENCY_INDEX ? index - 1 : index;
#  endif
	plugView *v = (plugView *)handle;
	v->ctrl->componentHandler->lpVtbl->beginEdit(v->ctrl->componentHandler, parameterInfo[index].id);
}

static void plugViewSetParameterCb(void *handle, size_t index, float value) {
	TRACE("set parameter cb\n");

#  ifdef DATA_PARAM_LATENCY_INDEX
	if (index == DATA_PARAM_LATENCY_INDEX)
		return;
	index = index >= DATA_PARAM_LATENCY_INDEX ? index - 1 : index;
#  endif
	plugView *v = (plugView *)handle;
	v->ctrl->parameters[index] = parameterAdjust(index, value); // let Reaper find the updated value
	v->ctrl->componentHandler->lpVtbl->performEdit(v->ctrl->componentHandler, parameterInfo[index].id, parameterUnmap(index, v->ctrl->parameters[index]));
}

static void plugViewSetParameterEndCb(void *handle, size_t index) {
	TRACE("set parameter end cb\n");

#  ifdef DATA_PARAM_LATENCY_INDEX
	if (index == DATA_PARAM_LATENCY_INDEX)
		return;
	index = index >= DATA_PARAM_LATENCY_INDEX ? index - 1 : index;
#  endif
	plugView *v = (plugView *)handle;
	v->ctrl->componentHandler->lpVtbl->endEdit(v->ctrl->componentHandler, parameterInfo[index].id);
}

# endif

# ifdef __APPLE__
static void plugViewTimerCb(CFRunLoopTimerRef timer, void *info) {
	(void)timer;

	plugin_ui_idle(((plugView *)info)->ui);
}
# elif defined(_WIN32) || defined(__CYGWIN__)
static void plugViewTimerCb(HWND p1, UINT p2, UINT_PTR p3, DWORD p4) {
	(void)p1;
	(void)p2;
	(void)p4;

	plugin_ui_idle(((plugView *)p3)->ui);
}
# endif

static Steinberg_tresult plugViewAttached(void* thisInterface, void* parent, Steinberg_FIDString type) {
	// GUI needs to be created here, see https://forums.steinberg.net/t/vst-and-hidpi/201916/3
	TRACE("plugView attached %p\n", thisInterface);

	if (plugViewIsPlatformTypeSupported(thisInterface, type) != Steinberg_kResultOk)
		return Steinberg_kInvalidArgument;

	plugView *v = (plugView *)((char *)thisInterface - offsetof(plugView, vtblIPlugView));
	if (v->ui)
		return Steinberg_kInvalidArgument;

	plugin_ui_callbacks cbs = {
		/* .handle        = */ (void *)v,
		/* .format        = */ "vst3",
		/* .get_bindir    = */ get_bindir_cb,
		/* .get_datadir   = */ get_datadir_cb,
# if DATA_PRODUCT_PARAMETERS_N > 0
		/* .set_parameter_begin	= */ plugViewSetParameterBeginCb,
		/* .set_parameter       = */ plugViewSetParameterCb,
		/* .set_parameter_end	= */ plugViewSetParameterEndCb
# else
		/* .set_parameter_begin	= */ NULL,
		/* .set_parameter       = */ NULL,
		/* .set_parameter_end	= */ NULL
# endif
	};
	v->ui = plugin_ui_create(1, parent, &cbs);
	if (!v->ui)
		return Steinberg_kResultFalse;

# ifdef __linux__
	v->display = XOpenDisplay(NULL);
	if (v->display == NULL) {
		plugin_ui_free(v->ui);
		v->ui = NULL;
		return Steinberg_kResultFalse;
	}
	if (v->runLoop->lpVtbl->registerTimer(v->runLoop, (struct Steinberg_ITimerHandler *)&v->timer, 20) != Steinberg_kResultOk) {
		XCloseDisplay(v->display);
		plugin_ui_free(v->ui);
		v->ui = NULL;
		return Steinberg_kResultFalse;
	}
# elif defined(__APPLE__)
	CFRunLoopTimerContext ctx = {
		/* .version         = */ 0,
		/* .info            = */ v,
		/* .retain          = */ NULL,
		/* .release         = */ NULL,
		/* .copyDescription = */ NULL
	};
	v->timer = CFRunLoopTimerCreate(NULL, CFAbsoluteTimeGetCurrent(), 20.0 / 1000.0, 0, 0, plugViewTimerCb, &ctx);
	CFRunLoopAddTimer(CFRunLoopGetCurrent(), v->timer, kCFRunLoopCommonModes);
# elif defined(_WIN32) || defined(__CYGWIN__)
	v->timer = SetTimer((HWND)*((char **)v->ui), (UINT_PTR)v, 20, (TIMERPROC)plugViewTimerCb);
	if (v->timer == 0) {
		plugin_ui_free(v->ui);
		v->ui = NULL;
		return Steinberg_kResultFalse;
	}
# endif
# if DATA_PRODUCT_PARAMETERS_N > 0
	plugViewUpdateAllParameters(v);
# endif
	return Steinberg_kResultTrue;
}

static Steinberg_tresult plugViewRemoved(void* thisInterface) {
	TRACE("plugView removed %p\n", thisInterface);
	plugView *v = (plugView *)((char *)thisInterface - offsetof(plugView, vtblIPlugView));
# ifdef __linux__
	v->runLoop->lpVtbl->unregisterTimer(v->runLoop, (struct Steinberg_ITimerHandler *)&v->timer);
	XCloseDisplay(v->display);
# elif defined(__APPLE__)
	CFRunLoopTimerInvalidate(v->timer);
	CFRelease(v->timer);
# elif defined(_WIN32) || defined(__CYGWIN__)
	KillTimer((HWND)(*((char **)v->ui)), (UINT_PTR)v);
# endif
	plugin_ui_free(v->ui);
	v->ui = NULL;
	return Steinberg_kResultTrue;
}

static Steinberg_tresult plugViewOnWheel(void* thisInterface, float distance) {
	(void)thisInterface;
	(void)distance;

	TRACE("plugView onWheel %p\n", thisInterface);
	//TODO
	return Steinberg_kResultFalse;
}

static Steinberg_tresult plugViewOnKeyDown(void* thisInterface, Steinberg_char16 key, Steinberg_int16 keyCode, Steinberg_int16 modifiers) {
	(void)thisInterface;
	(void)key;
	(void)keyCode;
	(void)modifiers;

	TRACE("plugView onKeyDown %p\n", thisInterface);
	//TODO
	return Steinberg_kResultFalse;
}

static Steinberg_tresult plugViewOnKeyUp(void* thisInterface, Steinberg_char16 key, Steinberg_int16 keyCode, Steinberg_int16 modifiers) {
	(void)thisInterface;
	(void)key;
	(void)keyCode;
	(void)modifiers;

	TRACE("plugView onKeyUp %p\n", thisInterface);
	//TODO
	return Steinberg_kResultFalse;
}

static Steinberg_tresult plugViewGetSize(void* thisInterface, struct Steinberg_ViewRect* size) {
	TRACE("plugView getSize %p %p\n", thisInterface, (void*) size);
	if (!size)
		return Steinberg_kInvalidArgument;
	plugView *v = (plugView *)((char *)thisInterface - offsetof(plugView, vtblIPlugView));
	size->left = 0;
	size->top = 0;
	if (!v->ui) {
		uint32_t width, height;
		plugin_ui_get_default_size(&width, &height);
		size->right = width;
		size->bottom = height;
	} else {
# ifdef __linux__
		XWindowAttributes attr;
		TRACE(" window %u\n", (Window)(*((char **)v->ui)));
		XGetWindowAttributes(v->display, (Window)(*((char **)v->ui)), &attr);
		size->right = attr.width;
		size->bottom = attr.height;
# endif
	}
	TRACE(" %u x %u\n", size->right, size->bottom);
	return Steinberg_kResultTrue;
}

static Steinberg_tresult plugViewOnSize(void* thisInterface, struct Steinberg_ViewRect* newSize) {
	TRACE("plugView onSize %p\n", thisInterface);
	plugView *v = (plugView *)((char *)thisInterface - offsetof(plugView, vtblIPlugView));
	// TODO: if not resizable by both user and plugin just return false
	if (!v->ui)
		return Steinberg_kResultFalse;
# ifdef __linux__
	TRACE(" window %u\n", (Window)(*((char **)v->ui)));
	XResizeWindow(v->display, (Window)(*((char **)v->ui)), newSize->right - newSize->left, newSize->bottom - newSize->top);
# elif defined(__APPLE__)
	CGSize size = { newSize->right - newSize->left, newSize->bottom - newSize->top };
	void (*f)(id, SEL, CGSize) = (void (*)(id, SEL, CGSize))objc_msgSend;
	f((id)(*((char **)v->ui)), sel_getUid("setFrameSize:"), size);
# elif defined(_WIN32) || defined(__CYGWIN__)
	SetWindowPos((HWND)*((char **)v->ui), HWND_TOP, 0, 0, newSize->right - newSize->left, newSize->bottom - newSize->top, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);
# endif
	return Steinberg_kResultTrue;
}

static Steinberg_tresult plugViewOnFocus(void* thisInterface, Steinberg_TBool state) {
	(void)thisInterface;
	(void)state;

	TRACE("plugView onFocus %p\n", thisInterface);
	//TODO
	return Steinberg_kResultFalse;
}

static Steinberg_tresult plugViewSetFrame(void* thisInterface, struct Steinberg_IPlugFrame* frame) {
	TRACE("plugView setFrame %p\n", thisInterface);
	plugView *v = (plugView *)((char *)thisInterface - offsetof(plugView, vtblIPlugView));
	v->frame = frame;
# ifdef __linux__
	if (v->frame) {
		if (v->frame->lpVtbl->queryInterface(v->frame, Steinberg_IRunLoop_iid, (void **)&v->runLoop) != Steinberg_kResultOk)
			return Steinberg_kResultFalse;
		v->frame->lpVtbl->release(v->frame);
	}
# endif
	return Steinberg_kResultTrue;
}

static Steinberg_tresult plugViewCanResize(void* thisInterface) {
	(void) thisInterface;

	TRACE("plugView canResize %p\n", thisInterface);
# if DATA_UI_USER_RESIZABLE
	return Steinberg_kResultTrue;
# else
	return Steinberg_kResultFalse;
# endif
}

static Steinberg_tresult plugViewCheckSizeConstraint(void* thisInterface, struct Steinberg_ViewRect* rect) {
	(void)thisInterface;
	(void)rect;

	TRACE("plugView chekSizeContraint %p\n", thisInterface);
# if DATA_UI_USER_RESIZABLE
	return Steinberg_kResultTrue;
# else
#  ifdef __linux__
	plugView *v = (plugView *)((char *)thisInterface - offsetof(plugView, vtblIPlugView));

	XWindowAttributes attr;
	XGetWindowAttributes(v->display, (Window)(*((char **)v->ui)), &attr);
	rect->right = rect->left + attr.width;
	rect->bottom = rect->top + attr.height;
#  endif
	return Steinberg_kResultFalse;
# endif
}

# ifdef __linux__
static void plugViewOnTimer(void *thisInterface) {
	TRACE("plugView onTimer %p\n", thisInterface);
	plugView *v = (plugView *)((char *)thisInterface - offsetof(plugView, vtblIPlugView));

	// Bitwig doesn't call onSize() as it should, so we compare the editor and parent window and resize if needed
	Window w = (Window)(*((char **)v->ui));
	Window root, parent, *children;
	unsigned nchildren;
	if (XQueryTree(v->display, w, &root, &parent, &children, &nchildren)) {
		if (children)
			XFree(children);
		XWindowAttributes parent_attr, editor_attr;
		XGetWindowAttributes(v->display, parent, &parent_attr);
		XGetWindowAttributes(v->display, w, &editor_attr);
		if (parent_attr.width != editor_attr.width || parent_attr.height != editor_attr.height)
			XResizeWindow(v->display, w, parent_attr.width, parent_attr.height);
	}

	plugin_ui_idle(v->ui);
}
# endif

static Steinberg_IPlugViewVtbl plugViewVtblIPlugView = {
	/* FUnknown */
	/* .queryInterface          = */ plugViewQueryInterface,
	/* .addRef                  = */ plugViewAddRef,
	/* .release                 = */ plugViewRelease,

	/* IPlugView */
	/* .isPlatformTypeSupported = */ plugViewIsPlatformTypeSupported,
	/* .attached                = */ plugViewAttached,
	/* .removed                 = */ plugViewRemoved,
	/* .onWheel                 = */ plugViewOnWheel,
	/* .onKeyDown               = */ plugViewOnKeyDown,
	/* .onKeyUp                 = */ plugViewOnKeyUp,
	/* .getSize                 = */ plugViewGetSize,
	/* .onSize                  = */ plugViewOnSize,
	/* .onFocus                 = */ plugViewOnFocus,
	/* .setFrame                = */ plugViewSetFrame,
	/* .canResize               = */ plugViewCanResize,
	/* .checkSizeConstraint     = */ plugViewCheckSizeConstraint
};
#endif

static Steinberg_Vst_IMidiMappingVtbl controllerVtblIMidiMapping;
#ifdef DATA_UI
//static Steinberg_Vst_IConnectionPointVtbl controllerVtblIConnectionPoint;
#endif

static Steinberg_tresult controllerQueryInterface(controller *c, const Steinberg_TUID iid, void ** obj) {
	// Same as above (pluginQueryInterface)
	size_t offset;
	if (memcmp(iid, Steinberg_FUnknown_iid, sizeof(Steinberg_TUID)) == 0
	    || memcmp(iid, Steinberg_IPluginBase_iid, sizeof(Steinberg_TUID)) == 0
	    || memcmp(iid, Steinberg_Vst_IEditController_iid, sizeof(Steinberg_TUID)) == 0)
		offset = offsetof(controller, vtblIEditController);
	else if (memcmp(iid, Steinberg_Vst_IMidiMapping_iid, sizeof(Steinberg_TUID)) == 0)
		offset = offsetof(controller, vtblIMidiMapping);
#ifdef DATA_UI
	/*else if (memcmp(iid, Steinberg_Vst_IConnectionPoint_iid, sizeof(Steinberg_TUID)) == 0)
		offset = offsetof(controller, vtblIConnectionPoint);*/
#endif
	else {
		TRACE(" not supported\n");
		for (int i = 0; i < 16; i++)
			TRACE(" %x", iid[i]);
		TRACE("\n");
		*obj = NULL;
		return Steinberg_kNoInterface;
	}
	*obj = (void *)((char *)c + offset);
	c->refs++;
	return Steinberg_kResultOk;
}

static Steinberg_uint32 controllerAddRef(controller *c) {
	c->refs++;
	return c->refs;
}

static Steinberg_uint32 controllerRelease(controller *c) {
	c->refs--;
	if (c->refs == 0) {
		TRACE(" free %p\n", (void *)c);
#ifdef DATA_UI
		if (c->views) {
			for (size_t i = 0; i < c->viewsCount; i++)
				if (c->views[i]) // this should not happen but you never know
					plugViewRelease(c->views[i]);
			free(c->views);
		}
#endif
		free(c);
		return 0;
	}
	return c->refs;
}

static Steinberg_tresult controllerIEditControllerQueryInterface(void* thisInterface, const Steinberg_TUID iid, void** obj) {
	TRACE("controller IEditController queryInterface %p\n", thisInterface);
	return controllerQueryInterface((controller *)((char *)thisInterface - offsetof(controller, vtblIEditController)), iid, obj);
}

static Steinberg_uint32 controllerIEditControllerAddRef(void* thisInterface) {
	TRACE("controller IEditController addRef %p\n", thisInterface);
	return controllerAddRef((controller *)((char *)thisInterface - offsetof(controller, vtblIEditController)));
}

static Steinberg_uint32 controllerIEditControllerRelease(void* thisInterface) {
	TRACE("controller IEditController release %p\n", thisInterface);
	return controllerRelease((controller *)((char *)thisInterface - offsetof(controller, vtblIEditController)));
}

static Steinberg_tresult controllerInitialize(void* thisInterface, struct Steinberg_FUnknown* context) {
	TRACE("controller initialize\n");
	controller *c = (controller *)((char *)thisInterface - offsetof(controller, vtblIEditController));
	if (c->context != NULL)
		return Steinberg_kResultFalse;
	c->context = context;
#if DATA_PRODUCT_PARAMETERS_N > 0
	for (int i = 0; i < DATA_PRODUCT_PARAMETERS_N; i++)
		c->parameters[i] = parameterData[i].def;
#endif
#if DATA_PRODUCT_BUSES_MIDI_INPUT_N > 0
	for (int i = DATA_PRODUCT_PARAMETERS_N; i < DATA_PRODUCT_PARAMETERS_N + 3 * DATA_PRODUCT_BUSES_MIDI_INPUT_N; i += 3) {
		c->parameters[i    ] = 0.0;
		c->parameters[i + 1] = 0.5;
		c->parameters[i + 2] = 0.0;
	}
#endif
	return Steinberg_kResultOk;
}

static Steinberg_tresult controllerTerminate(void* thisInterface) {
	TRACE("controller terminate\n");
	controller *c = (controller *)((char *)thisInterface - offsetof(controller, vtblIEditController));
	c->context = NULL;
	return Steinberg_kResultOk;
}

static Steinberg_tresult controllerSetComponentState(void* thisInterface, struct Steinberg_IBStream* state) {
	TRACE("controller set component state %p %p\n", thisInterface, (void *)state);
	if (state == NULL)
		return Steinberg_kResultFalse;
#if DATA_PRODUCT_PARAMETERS_N > 0
	controller *c = (controller *)((char *)thisInterface - offsetof(controller, vtblIEditController));
	for (size_t i = 0; i < DATA_PRODUCT_PARAMETERS_N; i++) {
		if (parameterInfo[i].flags & Steinberg_Vst_ParameterInfo_ParameterFlags_kIsReadOnly)
			continue;
		union { float f; uint32_t u; } v;
		Steinberg_int32 n;
		state->lpVtbl->read(state, &v, 4, &n);
		if (n != 4)
			return Steinberg_kResultFalse;
		if (IS_BIG_ENDIAN)
			v.u = SWAP_UINT32(v.u);
		c->parameters[i] = parameterAdjust(i, v.f);
	}
#endif
	TRACE(" ok\n");
	return Steinberg_kResultTrue;
}

static Steinberg_tresult controllerSetState(void* thisInterface, struct Steinberg_IBStream* state) {
	(void)thisInterface;
	(void)state;

	TRACE("controller set state\n");
	return Steinberg_kNotImplemented;
}

static Steinberg_tresult controllerGetState(void* thisInterface, struct Steinberg_IBStream* state) {
	(void)thisInterface;
	(void)state;

	TRACE("controller get state\n");
	return Steinberg_kNotImplemented;
}

static Steinberg_int32 controllerGetParameterCount(void* thisInterface) {
	(void)thisInterface;

	TRACE("controller get parameter count\n");
	return DATA_PRODUCT_PARAMETERS_N + 3 * DATA_PRODUCT_BUSES_MIDI_INPUT_N;
}

static Steinberg_tresult controllerGetParameterInfo(void* thisInterface, Steinberg_int32 paramIndex, struct Steinberg_Vst_ParameterInfo* info) {
	(void)thisInterface;

	TRACE("controller get parameter info\n");
	if (paramIndex < 0 || paramIndex >= DATA_PRODUCT_PARAMETERS_N + 3 * DATA_PRODUCT_BUSES_MIDI_INPUT_N)
		return Steinberg_kResultFalse;
	*info = parameterInfo[paramIndex];
	return Steinberg_kResultTrue;
}

static void dToStr(double v, Steinberg_Vst_String128 s, int precision) {
	int i = 0;

	if (v < 0.0) {
		s[0] = '-';
		v = -v;
		i++;
	}

	if (v < 1.0) {
		s[i] = '0';
		i++;
	} else {
		double x = 1.0;
		while (x <= v)
			x *= 10.0;
		x *= 0.1;
		while (x >= 1.0) {
			char c = v / x;
			s[i] = c + '0';
			i++;
			v -= c * x;
			x *= 0.1;
		}
	}

	s[i] = '.';
	i++;

	double x = 0.1;
	while (precision != 0) {
		char c = v / x;
		s[i] = c + '0';
		i++;
		v -= c * x;
		x *= 0.1;
		precision--;
	}
	
	s[i] = '\0';
}

static Steinberg_tresult controllerGetParamStringByValue(void* thisInterface, Steinberg_Vst_ParamID id, Steinberg_Vst_ParamValue valueNormalized, Steinberg_Vst_String128 string) {
	(void)thisInterface;

	TRACE("controller get param string by value\n");
	int pi = parameterGetIndexById(id);
	if (pi >= DATA_PRODUCT_PARAMETERS_N + 3 * DATA_PRODUCT_BUSES_MIDI_INPUT_N || pi < 0)
		return Steinberg_kResultFalse;
	dToStr(pi >= DATA_PRODUCT_PARAMETERS_N ? valueNormalized : parameterMap(pi, valueNormalized), string, 2);
	return Steinberg_kResultTrue;
}

void TCharToD(Steinberg_Vst_TChar* s, double *v) {
	int i = 0;
	*v = 0.0;

	if (s[0] == '-') {
		*v = -0.0;
		i++;
	}

	while (s[i] >= '0' && s[i] <= '9') {
		char d = s[i] - '0';
		i++;
		*v = 10.0 * *v + d;
	}

	if (s[i] != '.')
		return;
	i++;

	double x = 1.0;
	while (s[i] >= '0' && s[i] <= '9') {
		char d = s[i] - '0';
		i++;
		x *= 0.1;
		*v = *v + d * x;
	}
}

static Steinberg_tresult controllerGetParamValueByString(void* thisInterface, Steinberg_Vst_ParamID id, Steinberg_Vst_TChar* string, Steinberg_Vst_ParamValue* valueNormalized) {
	(void)thisInterface;

	TRACE("controller get param value by string\n");
	int pi = parameterGetIndexById(id);
	if (pi >= DATA_PRODUCT_PARAMETERS_N + 3 * DATA_PRODUCT_BUSES_MIDI_INPUT_N || pi < 0)
		return Steinberg_kResultFalse;
	double v;
	TCharToD(string, &v);
	*valueNormalized = pi >= DATA_PRODUCT_PARAMETERS_N ? v : parameterUnmap(pi, v);
	return Steinberg_kResultTrue;
}

static Steinberg_Vst_ParamValue controllerNormalizedParamToPlain(void* thisInterface, Steinberg_Vst_ParamID id, Steinberg_Vst_ParamValue valueNormalized) {
	(void)thisInterface;

	TRACE("controller normalized param to plain\n");
	int pi = parameterGetIndexById(id);
	return pi >= DATA_PRODUCT_PARAMETERS_N ? valueNormalized : parameterMap(pi, valueNormalized);
}

static Steinberg_Vst_ParamValue controllerPlainParamToNormalized(void* thisInterface, Steinberg_Vst_ParamID id, Steinberg_Vst_ParamValue plainValue) {
	(void)thisInterface;

	TRACE("controller plain param to normalized\n");
	int pi = parameterGetIndexById(id);
	return pi >= DATA_PRODUCT_PARAMETERS_N ? plainValue : parameterUnmap(pi, plainValue);
}

static Steinberg_Vst_ParamValue controllerGetParamNormalized(void* thisInterface, Steinberg_Vst_ParamID id) {
	TRACE("controller get param normalized\n");
#if DATA_PRODUCT_PARAMETERS_N + DATA_PRODUCT_BUSES_MIDI_INPUT_N > 0
	controller *c = (controller *)((char *)thisInterface - offsetof(controller, vtblIEditController));
	int pi = parameterGetIndexById(id);
# if DATA_PRODUCT_BUSES_MIDI_INPUT_N > 0
	return pi >= DATA_PRODUCT_PARAMETERS_N ? c->parameters[pi] : parameterUnmap(pi, c->parameters[pi]);
# else
	return parameterUnmap(pi, c->parameters[pi]);
# endif
#else
	(void)thisInterface;
	(void)id;
	return 0.0;
#endif
}

static Steinberg_tresult controllerSetParamNormalized(void* thisInterface, Steinberg_Vst_ParamID id, Steinberg_Vst_ParamValue value) {
	TRACE("controller set param normalized\n");
#if DATA_PRODUCT_PARAMETERS_N + DATA_PRODUCT_BUSES_MIDI_INPUT_N > 0
	int pi = parameterGetIndexById(id);
	if (pi >= DATA_PRODUCT_PARAMETERS_N + 3 * DATA_PRODUCT_BUSES_MIDI_INPUT_N || pi < 0)
		return Steinberg_kResultFalse;
	controller *c = (controller *)((char *)thisInterface - offsetof(controller, vtblIEditController));
	c->parameters[pi] = pi >= DATA_PRODUCT_PARAMETERS_N ? value : parameterAdjust(pi, parameterMap(pi, value));
# ifdef DATA_UI
	for (size_t i = 0; i < c->viewsCount; i++)
		if(c->views[i])
			plugViewUpdateParameter(c->views[i], pi);
# endif
	return Steinberg_kResultTrue;
#else
	(void)thisInterface;
	(void)id;
	(void)value;
	return Steinberg_kResultFalse;
#endif
}

static Steinberg_tresult controllerSetComponentHandler(void* thisInterface, struct Steinberg_Vst_IComponentHandler* handler) {
	TRACE("controller set component handler\n");
	controller *c = (controller *)((char *)thisInterface - offsetof(controller, vtblIEditController));
	if (c->componentHandler != handler) {
		if (c->componentHandler != NULL)
			c->componentHandler->lpVtbl->release(c->componentHandler);
		c->componentHandler = handler;
		if (c->componentHandler != NULL)
			c->componentHandler->lpVtbl->addRef(c->componentHandler);
	}
	return Steinberg_kResultTrue;
}

static struct Steinberg_IPlugView* controllerCreateView(void* thisInterface, Steinberg_FIDString name) {
	TRACE("controller create view %s\n", name);

#ifdef DATA_UI
	if (strcmp(name, "editor"))
		return NULL;

	plugView *view = malloc(sizeof(plugView));
	if (view == NULL)
		return NULL;

	controller *c = (controller *)((char *)thisInterface - offsetof(controller, vtblIEditController));
	size_t i;
	for (i = 0; i < c->viewsCount; c++)
		if (c->views[i] == NULL)
			break;
	if (i == c->viewsCount) {
		size_t cnt = i + 1;
		plugView **views = realloc(c->views, cnt * sizeof(plugView **));
		if (views == NULL) {
			free(view);
			return NULL;
		}
		c->views = views;
		c->viewsCount = cnt;
	}
	c->views[i] = view;

	view->vtblIPlugView = &plugViewVtblIPlugView;
	view->refs = 1;
	view->frame = NULL;
	view->ui = NULL;
	view->ctrl = c;
# ifdef __linux__
	view->timer.vtblITimerHandler = &timerHandlerVtblITimerHandler;
	view->timer.refs = 1;
	view->timer.cb = plugViewOnTimer;
	view->timer.data = view;
# endif

	return (struct Steinberg_IPlugView *)view;
#else
	(void)thisInterface;
	(void)name;

	return NULL;
#endif
}

static Steinberg_Vst_IEditControllerVtbl controllerVtblIEditController = {
	/* FUnknown */
	/* .queryInterface         = */ controllerIEditControllerQueryInterface,
	/* .addRef                 = */ controllerIEditControllerAddRef,
	/* .release                = */ controllerIEditControllerRelease,

	/* IPluginBase */
	/* .initialize             = */ controllerInitialize,
	/* .terminate              = */ controllerTerminate,

	/* IEditController */
	/* .setComponentState      = */ controllerSetComponentState,
	/* .setState               = */ controllerSetState,
	/* .getState               = */ controllerGetState,
	/* .getParameterCount      = */ controllerGetParameterCount,
	/* .getParameterInfo       = */ controllerGetParameterInfo,
	/* .getParamStringByValue  = */ controllerGetParamStringByValue,
	/* .getParamValueByString  = */ controllerGetParamValueByString,
	/* .normalizedParamToPlain = */ controllerNormalizedParamToPlain,
	/* .plainParamToNormalized = */ controllerPlainParamToNormalized,
	/* .getParamNormalized     = */ controllerGetParamNormalized,
	/* .setParamNormalized     = */ controllerSetParamNormalized,
	/* .setComponentHandler    = */ controllerSetComponentHandler,
	/* .createView             = */ controllerCreateView
};

static Steinberg_tresult controllerIMidiMappingQueryInterface(void* thisInterface, const Steinberg_TUID iid, void** obj) {
	TRACE("controller IMidiMapping queryInterface %p\n", thisInterface);
	return controllerQueryInterface((controller *)((char *)thisInterface - offsetof(controller, vtblIMidiMapping)), iid, obj);
}

static Steinberg_uint32 controllerIMidiMappingAddRef(void* thisInterface) {
	TRACE("controller IMidiMapping addRef %p\n", thisInterface);
	return controllerAddRef((controller *)((char *)thisInterface - offsetof(controller, vtblIMidiMapping)));
}

static Steinberg_uint32 controllerIMidiMappingRelease(void* thisInterface) {
	TRACE("controller IMidiMapping release %p\n", thisInterface);
	return controllerRelease((controller *)((char *)thisInterface - offsetof(controller, vtblIMidiMapping)));
}

static Steinberg_tresult controllerGetMidiControllerAssignment(void* thisInterface, Steinberg_int32 busIndex, Steinberg_int16 channel, Steinberg_Vst_CtrlNumber midiControllerNumber, Steinberg_Vst_ParamID* id) {
	(void)thisInterface;
	(void)channel;

	TRACE("controller getMidiControllerAssignment\n");
	if (busIndex < 0 || busIndex >= DATA_PRODUCT_BUSES_MIDI_INPUT_N)
		return Steinberg_kInvalidArgument;
	switch (midiControllerNumber) {
	case Steinberg_Vst_ControllerNumbers_kAfterTouch:
		*id = parameterInfo[DATA_PRODUCT_PARAMETERS_N + 3 * busIndex].id;
		return Steinberg_kResultTrue;
		break;
	case Steinberg_Vst_ControllerNumbers_kPitchBend:
		*id = parameterInfo[DATA_PRODUCT_PARAMETERS_N + 3 * busIndex + 1].id;
		return Steinberg_kResultTrue;
		break;
	case Steinberg_Vst_ControllerNumbers_kCtrlModWheel:
		*id = parameterInfo[DATA_PRODUCT_PARAMETERS_N + 3 * busIndex + 2].id;
		return Steinberg_kResultTrue;
		break;
	default:
		return Steinberg_kResultFalse;
		break;
	}
}

static Steinberg_Vst_IMidiMappingVtbl controllerVtblIMidiMapping = {
	/* FUnknown */
	/* .queryInterface              = */ controllerIMidiMappingQueryInterface,
	/* .addRef                      = */ controllerIMidiMappingAddRef,
	/* .release                     = */ controllerIMidiMappingRelease,

	/* IMidiMapping */
	/* .getMidiControllerAssignment = */ controllerGetMidiControllerAssignment
};

#ifdef DATA_UI
# if 0
static Steinberg_tresult controllerIConnectionPointQueryInterface(void* thisInterface, const Steinberg_TUID iid, void** obj) {
	TRACE("controller IConnectionPoint queryInterface %p\n", thisInterface);
	return controllerQueryInterface((controller *)((char *)thisInterface - offsetof(controller, vtblIConnectionPoint)), iid, obj);
}

static Steinberg_uint32 controllerIConnectionPointAddRef(void* thisInterface) {
	TRACE("controller IConnectionPoint addRef %p\n", thisInterface);
	return controllerAddRef((controller *)((char *)thisInterface - offsetof(controller, vtblIConnectionPoint)));
}

static Steinberg_uint32 controllerIConnectionPointRelease(void* thisInterface) {
	TRACE("controller IConnectionPoint release %p\n", thisInterface);
	return controllerRelease((controller *)((char *)thisInterface - offsetof(controller, vtblIConnectionPoint)));
}

static Steinberg_tresult controllerIConnectionPointConnect(void* thisInterface, struct Steinberg_Vst_IConnectionPoint* other) {
	(void)thisInterface;

	return other ? Steinberg_kResultOk : Steinberg_kInvalidArgument;
}

static Steinberg_tresult controllerIConnectionPointDisconnect(void* thisInterface, struct Steinberg_Vst_IConnectionPoint* other) {
	(void)thisInterface;
	(void)other;

	return Steinberg_kResultOk;
}

static Steinberg_tresult controllerIConnectionPointNotify(void* thisInterface, struct Steinberg_Vst_IMessage* message) {
	(void)thisInterface;
	(void)message;

	return Steinberg_kResultOk;
}

static Steinberg_Vst_IConnectionPointVtbl controllerVtblIConnectionPoint = {
	/* FUnknown */
	/* .queryInterface = */ controllerIConnectionPointQueryInterface,
	/* .addRef         = */ controllerIConnectionPointAddRef,
	/* .release        = */ controllerIConnectionPointRelease,

	/* IConnectionPoint */
	/* .connect        = */ controllerIConnectionPointConnect,
	/* .disconnect     = */ controllerIConnectionPointDisconnect,
	/* .notify         = */ controllerIConnectionPointNotify
};
# endif
#endif

static Steinberg_tresult factoryQueryInterface(void *thisInterface, const Steinberg_TUID iid, void ** obj) {
	TRACE("factory queryInterface\n");
	if (memcmp(iid, Steinberg_FUnknown_iid, sizeof(Steinberg_TUID))
	    && memcmp(iid, Steinberg_IPluginFactory_iid, sizeof(Steinberg_TUID))
	    && memcmp(iid, Steinberg_IPluginFactory2_iid, sizeof(Steinberg_TUID))
	    && memcmp(iid, Steinberg_IPluginFactory3_iid, sizeof(Steinberg_TUID))) {
		TRACE(" not supported\n");
		*obj = NULL;
		return Steinberg_kNoInterface;
	}
	*obj = thisInterface;
	return Steinberg_kResultOk;
}

static Steinberg_uint32 factoryAddRef(void *thisInterface) {
	(void)thisInterface;

	TRACE("factory add ref\n");
	return 1;
}

static Steinberg_uint32 factoryRelease(void *thisInterface) {
	(void)thisInterface;

	TRACE("factory release\n");
	return 1;
}

static Steinberg_tresult factoryGetFactoryInfo(void *thisInterface, struct Steinberg_PFactoryInfo * info) {
	(void)thisInterface;

	TRACE("getFactoryInfo\n");
	strcpy(info->vendor, DATA_COMPANY_NAME);
	strcpy(info->url, DATA_COMPANY_URL);
	strcpy(info->email, DATA_COMPANY_EMAIL);
	info->flags = Steinberg_PFactoryInfo_FactoryFlags_kUnicode;
	return Steinberg_kResultOk;
}

static Steinberg_int32 factoryCountClasses(void *thisInterface) {
	(void)thisInterface;

	TRACE("countClasses\n");
	return 2;
}

static Steinberg_tresult factoryGetClassInfo(void *thisInterface, Steinberg_int32 index, struct Steinberg_PClassInfo * info) {
	(void)thisInterface;

	TRACE("getClassInfo\n");
	switch (index) {
	case 0:
		TRACE(" class 0\n");
		memcpy(info->cid, dataPluginCID, sizeof(Steinberg_TUID));
		info->cardinality = Steinberg_PClassInfo_ClassCardinality_kManyInstances;
		strcpy(info->category, "Audio Module Class");
		strcpy(info->name, DATA_PRODUCT_NAME);
		break;
	case 1:
		TRACE(" class 1\n");
		memcpy(info->cid, dataControllerCID, sizeof(Steinberg_TUID));
		info->cardinality = Steinberg_PClassInfo_ClassCardinality_kManyInstances;
		strcpy(info->category, "Component Controller Class");
		strcpy(info->name, DATA_PRODUCT_NAME " Controller");
		break;
	default:
		return Steinberg_kInvalidArgument;
		break;
	}
	return Steinberg_kResultOk;
}

static Steinberg_tresult factoryCreateInstance(void *thisInterface, Steinberg_FIDString cid, Steinberg_FIDString iid, void ** obj) {
	(void)thisInterface;

	TRACE("createInstance\n");
	if (memcmp(cid, dataPluginCID, sizeof(Steinberg_TUID)) == 0) {
		TRACE(" plugin\n");
		size_t offset; // does it actually work like this? or is offset always 0? this seems to be correct and works...
		if ((memcmp(iid, Steinberg_FUnknown_iid, sizeof(Steinberg_TUID)) == 0)
		    || (memcmp(iid, Steinberg_IPluginBase_iid, sizeof(Steinberg_TUID)) != 0)
		    || (memcmp(iid, Steinberg_Vst_IComponent_iid, sizeof(Steinberg_TUID)) != 0)) {
			TRACE("  IComponent\n");
			offset = offsetof(pluginInstance, vtblIComponent);
		} else if (memcmp(iid, Steinberg_Vst_IAudioProcessor_iid, sizeof(Steinberg_TUID)) != 0) {
			TRACE("  IAudioProcessor\n");
			offset = offsetof(pluginInstance, vtblIAudioProcessor);
		} else if (memcmp(iid, Steinberg_Vst_IProcessContextRequirements_iid, sizeof(Steinberg_TUID)) != 0) {
			TRACE("  IProcessContextRequirements\n");
			offset = offsetof(pluginInstance, vtblIProcessContextRequirements);
		} else {
			TRACE("  INothing :(\n");
			for (int i = 0; i < 16; i++)
				TRACE(" %x", iid[i]);
			TRACE("\n");
			return Steinberg_kNoInterface;
		}
		pluginInstance *p = malloc(sizeof(pluginInstance));
		if (p == NULL)
			return Steinberg_kOutOfMemory;
		p->vtblIComponent = &pluginVtblIComponent;
		p->vtblIAudioProcessor = &pluginVtblIAudioProcessor;
		p->vtblIProcessContextRequirements = &pluginVtblIProcessContextRequirements;
		p->refs = 1;
		p->context = NULL;
		*obj = (void *)((char *)p + offset);
		TRACE(" instance: %p\n", (void *)p);
	} else if (memcmp(cid, dataControllerCID, sizeof(Steinberg_TUID)) == 0) {
		TRACE(" controller\n");
		if (memcmp(iid, Steinberg_FUnknown_iid, sizeof(Steinberg_TUID))
		    && memcmp(iid, Steinberg_IPluginBase_iid, sizeof(Steinberg_TUID))
		    && memcmp(iid, Steinberg_Vst_IEditController_iid, sizeof(Steinberg_TUID)))
			return Steinberg_kNoInterface;
		controller *c = malloc(sizeof(controller));
		if (c == NULL)
			return Steinberg_kOutOfMemory;
		c->vtblIEditController = &controllerVtblIEditController;
		c->vtblIMidiMapping = &controllerVtblIMidiMapping;
#ifdef DATA_UI
		//c->vtblIConnectionPoint = &controllerVtblIConnectionPoint;
#endif
		c->refs = 1;
		c->context = NULL;
		c->componentHandler = NULL;
#ifdef DATA_UI
		c->views = NULL;
		c->viewsCount = 0;
#endif
		*obj = c;
		TRACE(" instance: %p\n", (void *)c);
	} else {
		*obj = NULL;
		return Steinberg_kNoInterface;
	}
	return Steinberg_kResultOk;
}

static Steinberg_tresult factoryGetClassInfo2(void* thisInterface, Steinberg_int32 index, struct Steinberg_PClassInfo2* info) {
	(void)thisInterface;

	TRACE("getClassInfo2\n");
	switch (index) {
	case 0:
		TRACE(" class 0\n");
		memcpy(info->cid, dataPluginCID, sizeof(Steinberg_TUID));
		info->cardinality = Steinberg_PClassInfo_ClassCardinality_kManyInstances;
		strcpy(info->category, "Audio Module Class");
		strcpy(info->name, DATA_PRODUCT_NAME);
		info->classFlags = Steinberg_Vst_ComponentFlags_kDistributable;
		strcpy(info->subCategories, DATA_VST3_SUBCATEGORY);
		*info->vendor = '\0';
		strcpy(info->version, DATA_PRODUCT_VERSION);
		strcpy(info->sdkVersion, "VST " DATA_VST3_SDK_VERSION " | Tibia");
		break;
	case 1:
		TRACE(" class 1\n");
		memcpy(info->cid, dataControllerCID, sizeof(Steinberg_TUID));
		info->cardinality = Steinberg_PClassInfo_ClassCardinality_kManyInstances;
		strcpy(info->category, "Component Controller Class");
		strcpy(info->name, DATA_PRODUCT_NAME " Controller");
		info->classFlags = 0;
		*info->subCategories = '\0';
		*info->vendor = '\0';
		strcpy(info->version, DATA_PRODUCT_VERSION);
		strcpy(info->sdkVersion, "VST " DATA_VST3_SDK_VERSION " | Tibia");
		break;
	default:
		return Steinberg_kInvalidArgument;
		break;
	}
	return Steinberg_kResultOk;
}

static Steinberg_tresult factoryGetClassInfoUnicode(void* thisInterface, Steinberg_int32 index, struct Steinberg_PClassInfoW* info) {
	(void)thisInterface;

	TRACE("getClassInfo unicode\n");
	switch (index) {
	case 0:
		TRACE(" class 0\n");
		memcpy(info->cid, dataPluginCID, sizeof(Steinberg_TUID));
		info->cardinality = Steinberg_PClassInfo_ClassCardinality_kManyInstances;
		strcpy(info->category, "Audio Module Class");
		memcpy(info->name, dataProductNameW, 64 * sizeof(Steinberg_char16));
		info->classFlags = Steinberg_Vst_ComponentFlags_kDistributable;
		strcpy(info->subCategories, DATA_VST3_SUBCATEGORY);
		*info->vendor = '\0';
		memcpy(info->version, dataProductVersionW, 64 * sizeof(Steinberg_char16));
		memcpy(info->sdkVersion, dataVST3SDKVersionW, 64 * sizeof(Steinberg_char16));
		break;
	case 1:
		TRACE(" class 1\n");
		memcpy(info->cid, dataControllerCID, sizeof(Steinberg_TUID));
		info->cardinality = Steinberg_PClassInfo_ClassCardinality_kManyInstances;
		strcpy(info->category, "Component Controller Class");
		memcpy(info->name, dataVST3ControllerNameW, 64 * sizeof(Steinberg_char16));
		info->classFlags = 0;
		*info->subCategories = '\0';
		*info->vendor = '\0';
		memcpy(info->version, dataProductVersionW, 64 * sizeof(Steinberg_char16));
		memcpy(info->sdkVersion, dataVST3SDKVersionW, 64 * sizeof(Steinberg_char16));
		break;
	default:
		return Steinberg_kInvalidArgument;
		break;
	}
	return Steinberg_kResultOk;
}

static Steinberg_tresult factorySetHostContext(void* thisInterface, struct Steinberg_FUnknown* context) {
	(void)thisInterface;
	(void)context;

	TRACE("factory set host context %p %p\n", thisInterface, (void*) context);

	return Steinberg_kNotImplemented;
}

static Steinberg_IPluginFactory3Vtbl factoryVtbl = {
	/* FUnknown */
	/* .queryInterface      = */ factoryQueryInterface,
	/* .addRef              = */ factoryAddRef,
	/* .release             = */ factoryRelease,

	/* IPluginFactory */
	/* .getFactoryInfo      = */ factoryGetFactoryInfo,
	/* .countClasses        = */ factoryCountClasses,
	/* .getClassInfo        = */ factoryGetClassInfo,
	/* .createInstance      = */ factoryCreateInstance,

	/* IPluginFactory2 */
	/* .getClassInfo2       = */ factoryGetClassInfo2,

	/* IPluginFactory3 */
	/* .getClassInfoUnicode = */ factoryGetClassInfoUnicode,
	/* .setHostContext      = */ factorySetHostContext
};
static Steinberg_IPluginFactory3 factory = { &factoryVtbl };

#if defined(_WIN32) || defined(__CYGWIN__)
# define EXPORT __declspec(dllexport)
#else
# define EXPORT __attribute__((visibility("default")))
#endif

EXPORT
Steinberg_IPluginFactory * GetPluginFactory(void) {
	return (Steinberg_IPluginFactory *)&factory;
}

static int refs = 0; 

static char vstExit(void) {
	refs--;
	if (refs == 0) {
		free(bindir);
		free(datadir);
		return 0;
	}
	return 1;
}

#if defined(_WIN32) || defined(__CYGWIN__)

EXPORT APIENTRY BOOL
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved) {
	(void)hInstance;
	(void)lpReserved;
	if (dwReason == DLL_PROCESS_ATTACH) {
		if (refs == 0) {
			int pathLength;
			char path[260];
			HMODULE hm = NULL;
			if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR) &bindir, &hm) == 0) {
			    TRACE("GetModuleHandle failed, error = %lu\n", GetLastError());
			    return 0;
			}
			if ((pathLength = GetModuleFileName(hm, path, sizeof(path))) == 0) {
			    TRACE("GetModuleFileName failed, error = %lu\n", GetLastError());
			    return 0;
			}
			char *c = strrchr(path, '\\');
			*c = '\0';
			bindir = (char*) malloc(strlen(path) + 1);
			memcpy(bindir, path, strlen(path));
			bindir[strlen(path)] = '\0';
			c = strrchr(path, '\\');
			*c = '\0';
			static char* res = "\\Resources";
			datadir = malloc(strlen(path) + strlen(res) + 1);
			sprintf(datadir, "%s%s", path, res);
			TRACE("bindir = %s \ndatadir = %s\n", bindir, datadir);
		}
		refs++;
	} else if (dwReason == DLL_PROCESS_DETACH) {
		vstExit();
	}
	return 1;
}

#elif defined(__APPLE__) || defined(__linux__)

EXPORT
# if defined(__APPLE__)
char bundleEntry(CFBundleRef ref) {
	(void)ref;
# else
char ModuleEntry(void *handle) {
	(void)handle;
# endif
	char *file;
	if (refs == 0) {
		Dl_info info;
		union { void* d; char (*f)(void); } v;
		v.f = vstExit;
		if (dladdr((void*) v.d, &info) == 0)
			return 0;
		file = realpath(info.dli_fname, NULL);
		if (file == NULL)
			return 0;
		char *c = strrchr(file, '/');
		*c = '\0';
		bindir = strdup(file);
		if (bindir == NULL)
			goto err_bindir;
		c = strrchr(file, '/');
		*c = '\0';
		datadir = x_asprintf("%s/Resources", file);
		if (datadir == NULL)
			goto err_datadir;
		free(file);
		TRACE("bindir = %s \ndatadir = %s\n", bindir, datadir);
	}
	refs++;
	return 1;

err_datadir:
	free(bindir);
err_bindir:
	free(file);
	return 0;
}

EXPORT
# if defined(__APPLE__)
char bundleExit(void) {
# else
char ModuleExit(void) {
# endif
	return vstExit();
}

#endif
