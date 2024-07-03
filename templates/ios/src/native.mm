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

typedef struct {
	void       *handle;
	const char *format;
	const char *(*get_bindir)(void *handle);
	const char *(*get_datadir)(void *handle);
} plugin_callbacks;

#include "data.h"
#include "plugin.h"
#if PARAMETERS_N > 0
#include <algorithm>
#endif
#if PARAMETERS_N + NUM_MIDI_INPUTS > 0
#include <mutex>
#endif
#include <vector>
#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_RUNTIME_LINKING
#include "miniaudio.h"
#define BLOCK_SIZE  32

static ma_device        device;
static ma_device_config deviceConfig;
char                    device_inited = 0;
static plugin           instance;
static void            *mem;
#if NUM_NON_OPT_CHANNELS_IN > NUM_CHANNELS_IN
float                   zero[BLOCK_SIZE];
#endif
#if NUM_CHANNELS_IN > 0
float                   x_buf[NUM_CHANNELS_IN * BLOCK_SIZE];
float                  *x_in[NUM_CHANNELS_IN];
#endif
#if NUM_ALL_CHANNELS_IN > 0
const float            *x[NUM_ALL_CHANNELS_IN];
#else
const float           **x;
#endif
#if NUM_NON_OPT_CHANNELS_OUT > 0
float                   y_buf[NUM_NON_OPT_CHANNELS_OUT * BLOCK_SIZE];
#endif
#if NUM_CHANNELS_OUT > 0
float                  *y_out[NUM_CHANNELS_OUT];
#endif
#if NUM_ALL_CHANNELS_OUT > 0
float                  *y[NUM_ALL_CHANNELS_OUT];
#else
float                 **y;
#endif
#if PARAMETERS_N > 0
std::mutex              mutex;
float                   param_values[PARAMETERS_N];
float                   param_values_prev[PARAMETERS_N];
#endif
#if NUM_MIDI_INPUTS > 0
CFStringRef             midiClientName = NULL;
MIDIClientRef           midiClient = NULL;
CFStringRef             midiInputName = NULL;
MIDIPortRef             midiPort = NULL;
#define MIDIBUFFERLEN 1023
uint8_t                 midiBuffer[MIDIBUFFERLEN];
int                     midiBuffer_i = 0;
#endif

static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
	(void)pDevice;
	
#if defined(__aarch64__)
	uint64_t fpcr;
	__asm__ __volatile__ ("mrs %0, fpcr" : "=r"(fpcr));
	__asm__ __volatile__ ("msr fpcr, %0" :: "r"(fpcr | 0x1000000)); // enable FZ
#endif

#if PARAMETERS_N + NUM_MIDI_INPUTS > 0
	if (mutex.try_lock()) {
# if PARAMETERS_N > 0
		for (size_t i = 0; i < PARAMETERS_N; i++) {
			if (param_data[i].out)
				param_values_prev[i] = param_values[i] = plugin_get_parameter(&instance, i);
			else if (param_values_prev[i] != param_values[i]) {
				plugin_set_parameter(&instance, i, param_values[i]);
				param_values_prev[i] = param_values[i];
			}
		}
# endif

# if NUM_MIDI_INPUTS > 0
		if (midiBuffer_i > 0) {
			for (int i = 0; i < midiBuffer_i; i+=3) {
				plugin_midi_msg_in(&instance, MIDI_BUS_IN, &(midiBuffer[i]));
			}
		}
		midiBuffer_i = 0;
# endif
		mutex.unlock();
	}
#endif
	
#if NUM_CHANNELS_IN > 0
	const float * in_buf = reinterpret_cast<const float *>(pInput);
	size_t ix = 0;
#else
	(void)pInput;
#endif
#if NUM_CHANNELS_OUT > 0
	float * out_buf = reinterpret_cast<float *>(pOutput);
	size_t iy = 0;
#else
	(void)pOutput;
#endif
	ma_uint32 i = 0;
	while (i < frameCount) {
		ma_uint32 n = std::min(frameCount - i, static_cast<ma_uint32>(BLOCK_SIZE));

#if NUM_CHANNELS_IN > 0
		for (ma_uint32 j = 0; j < n; j++)
			for (size_t k = 0;  k < NUM_CHANNELS_IN; k++, ix++)
				x_in[k][j] = in_buf[ix];
#endif

		plugin_process(&instance, x, y, n);

#if NUM_CHANNELS_OUT > 0
		for (ma_uint32 j = 0; j < n; j++)
			for (size_t k = 0;  k < NUM_CHANNELS_OUT; k++, iy++)
				out_buf[iy] = y_out[k][j];
#elif NUM_CHANNELS_IN == 0
		for (ma_uint32 j = 0; j < n; j++)
			out_buf[j] = 0.f;
#endif

		i += n;
	}

#if defined(__aarch64__)
	__asm__ __volatile__ ("msr fpcr, %0" : : "r"(fpcr));
#endif
}

#if (NUM_MIDI_INPUTS > 0)
void (^midiNotifyBlock)(const MIDINotification *message) = ^(const MIDINotification *message) {
	if (message->messageID != kMIDIMsgObjectAdded)
		return;
	const MIDIObjectAddRemoveNotification *n = reinterpret_cast<const MIDIObjectAddRemoveNotification *>(message);
	MIDIEndpointRef endPoint = n->child;
	MIDIPortConnectSource(midiPort, endPoint, NULL);
};

void (^midiReceiveBlock)(const MIDIEventList *evtlist, void *srcConnRefCon) = ^(const MIDIEventList *evtlist, void *srcConnRefCon) {
	const MIDIEventPacket *p = evtlist->packet;
	for (UInt32 i = 0; i < evtlist->numPackets; i++) {
		for (UInt32 j = 0; j < p->wordCount; j++) {
			const UInt32 w = p->words[j];
			const uint8_t* t = (uint8_t*) &(w);
			
			if ((t[3] & 0xf0) != 32)
				continue; // We only support MIDI 1.0
			if ((t[2] & 0xF0) == 0xF0)
				continue;
			mutex.lock();
			if (midiBuffer_i < MIDIBUFFERLEN - 3) {
				midiBuffer[midiBuffer_i    ] = t[2];
				midiBuffer[midiBuffer_i + 1] = t[1];
				midiBuffer[midiBuffer_i + 2] = t[0];
				midiBuffer_i += 3;
			}
			mutex.unlock();
		}
		p = MIDIEventPacketNext(p);
	}
};
#endif

extern "C"
char audioStart() {
#if NUM_CHANNELS_IN + NUM_CHANNELS_OUT > 0
# if NUM_CHANNELS_IN == 0
	deviceConfig = ma_device_config_init(ma_device_type_playback);
# elif NUM_CHANNELS_OUT == 0
	deviceConfig = ma_device_config_init(ma_device_type_capture);
# else
	deviceConfig = ma_device_config_init(ma_device_type_duplex);
# endif
#else
	deviceConfig = ma_device_config_init(ma_device_type_playback);
#endif

	deviceConfig.periodSizeInFrames        = BLOCK_SIZE;
	deviceConfig.periods                   = 1;
	deviceConfig.performanceProfile        = ma_performance_profile_low_latency;
	deviceConfig.noPreSilencedOutputBuffer = 1;
	deviceConfig.noClip                    = 0;
	deviceConfig.noDisableDenormals        = 0;
	deviceConfig.noFixedSizedCallback      = 1;
	deviceConfig.dataCallback              = data_callback;
	deviceConfig.capture.pDeviceID         = NULL;
	deviceConfig.capture.format            = ma_format_f32;
	deviceConfig.capture.channels          = NUM_CHANNELS_IN;
	deviceConfig.capture.shareMode         = ma_share_mode_shared;
	deviceConfig.playback.pDeviceID        = NULL;
	deviceConfig.playback.format           = ma_format_f32;
#if NUM_CHANNELS_IN + NUM_CHANNELS_OUT > 0
	deviceConfig.playback.channels         = NUM_CHANNELS_OUT;
#else
	deviceConfig.playback.channels         = 1; // Fake & muted
#endif
	deviceConfig.playback.shareMode        = ma_share_mode_shared;

	if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS)
		return false;

#if (NUM_MIDI_INPUTS > 0)
	if (midiClientName == NULL) {
		midiClientName = CFSTR("template");
		if (midiClientName == NULL)
			return false; // Check unint
	}
	if (midiClient == (MIDIClientRef)NULL) {
		if (MIDIClientCreateWithBlock(midiClientName, &midiClient, midiNotifyBlock) != 0)
			return false;
	}
	if (midiInputName == NULL) {
		midiInputName = CFSTR("Input");
		if (midiInputName == NULL)
			return false;
	}
	if (midiPort == (MIDIPortRef)NULL) {
		if (MIDIInputPortCreateWithProtocol(midiClient, midiInputName, kMIDIProtocol_1_0, &midiPort, midiReceiveBlock) != 0)
			return false;

		ItemCount n = MIDIGetNumberOfSources();
		for (ItemCount i = 0; i < n; i++) {
			MIDIEndpointRef endPoint = MIDIGetSource(i);
			MIDIPortConnectSource(midiPort, endPoint, NULL);
		}
	}
#endif

	plugin_callbacks cbs = {
		/* .handle      = */ NULL,
		/* .format      = */ "ios",
		/* .get_bindir  = */ NULL,
		/* .get_datadir = */ NULL
	};
	plugin_init(&instance, &cbs);

#if PARAMETERS_N > 0
	for (size_t i = 0; i < PARAMETERS_N; i++) {
		if (!param_data[i].out)
			plugin_set_parameter(&instance, i, param_data[i].def);
		param_values_prev[i] = param_values[i] = param_data[i].def;
	}
#endif
	
	plugin_set_sample_rate(&instance, (float)device.sampleRate);

	size_t req = plugin_mem_req(&instance);
	if (req != 0) {
		mem = malloc(req);
		if (mem == NULL) {
			plugin_fini(&instance);
			ma_device_uninit(&device);
			return false;
		}
		plugin_mem_set(&instance, mem);
	} else
		mem = NULL;

	plugin_reset(&instance);

#if NUM_ALL_CHANNELS_IN > 0
	for (size_t i = 0, j = 0, k = 0; i < NUM_AUDIO_BUSES_IN + NUM_AUDIO_BUSES_OUT; i++) {
		if (audio_bus_data[i].out)
			continue;
		for (int l = 0; l < audio_bus_data[i].channels; l++, j++) {
			if (AUDIO_BUS_IN == audio_bus_data[i].index) {
				float * b = x_buf + BLOCK_SIZE * k;
				x[j] = b;
				x_in[l] = b;
				k++;
			} else
#if NUM_NON_OPT_CHANNELS_IN > NUM_CHANNELS_IN
				x[j] = audio_bus_data[i].optional ? NULL : zero;
#else
				x[j] = NULL;
#endif
		}
	}
#else
	x = NULL;
#endif

#if NUM_ALL_CHANNELS_OUT > 0
	for (size_t i = 0, j = 0, k = 0; i < NUM_AUDIO_BUSES_IN + NUM_AUDIO_BUSES_OUT; i++) {
		if (!audio_bus_data[i].out)
			continue;
		for (int l = 0; l < audio_bus_data[i].channels; l++, j++) {
			if (AUDIO_BUS_OUT == audio_bus_data[i].index) {
				y[j] = y_buf + BLOCK_SIZE * k;
				y_out[l] = y[j];
				k++;
			} else if (!audio_bus_data[i].optional) {
				y[j] = y_buf + BLOCK_SIZE * k;
				k++;
			} else
				y[j] = NULL; 
		}
	}
#else
	y = NULL;
#endif

#if NUM_NON_OPT_CHANNELS_IN > NUM_CHANNELS_IN
	memset(zero, 0, BLOCK_SIZE * sizeof(float));
#endif

	if (ma_device_start(&device) != MA_SUCCESS) {
		if (mem != NULL)
			free(mem);
		ma_device_uninit(&device);
		return false;
	}

	device_inited = 1;

	return true;
}

extern "C"
void audioStop() {
	ma_device_stop(&device);
	ma_device_uninit(&device);
	if (mem != NULL)
		free(mem);
	plugin_fini(&instance);
	// No need to close MIDI connections (e.g. via MIDIClientDispose), the system terminates them when the app terminates.
}

extern "C"
void audioPause() {
	if (device_inited) {
		ma_device_stop(&device);
		ma_device_uninit(&device);
	}
}

extern "C"
void audioResume() {
	// TODO: could this fail?...
	if (device_inited) {
		ma_device_init(NULL, &deviceConfig, &device);
		ma_device_start(&device);
	}
}

extern "C"
float getParameter(int i) {
#if PARAMETERS_N > 0
	mutex.lock();
	float v = param_values[i];
	mutex.unlock();
	return v;
#else
	(void)i;
	return 0.f;
#endif
}

extern "C"
void setParameter(int i, float v) {
	if (param_data[i].flags & (PARAM_BYPASS | PARAM_TOGGLED))
		v = v > 0.5f ? 1.f : 0.f;
	else if (param_data[i].flags & PARAM_INTEGER)
		v = (int32_t)(v + (v >= 0.f ? 0.5f : -0.5f));
	v = std::min(std::max(v, param_data[i].min), param_data[i].max);
#if PARAMETERS_N > 0
	mutex.lock();
	param_values[i] = v;
	mutex.unlock();
#endif
}
