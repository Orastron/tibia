/*
 * Tibia
 *
 * Copyright (C) 2023-2026 Orastron Srl unipersonale
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

#include <stdint.h>
#include <stdio.h>
#include <new>

class Plugin {
public:
	Plugin(void * handle, void (*msg_write)(void *handle, size_t size, const void *data)) {
		mHandle = handle;
		mMsgWrite = msg_write;
	}

	void setSampleRate(float value) {
		mSampleRate = value;
		//safe approx delay_line_length = ceilf(sample_rate) + 1;
		mDelayLineLength = (size_t)(mSampleRate + 1.f) + 1;
	}

	size_t memReq() {
		return mDelayLineLength * sizeof(float);
	}

	void memSet(void * mem) {
		mDelayLine = (float *)mem;
	}

	void reset() {
		for (size_t i = 0; i < mDelayLineLength; i++)
			mDelayLine[i] = 0.f;
		mDelayLineCur = 0;
		mZ1 = 0.f;
		mCutoffK = 1.f;
		mPlaying = false;
		mSpeed = 1.f;
		mSpeedK = 1.f;
		mBPM = 120.f;
		mPhase = 0.f;
		mYZ1 = 0.f;
		mCount = 0;
	}

	void setGain(float value) {
		mGain = value;
	}

	void setDelay(float value) {
		mDelay = value;
	}

	void setCutoff(float value) {
		mCutoff = value;
	}

	void setTremolo(float value) {
		mTremolo = value;
	}

	void setBypass(bool value) {
		mBypass = value;
	}

	float getGain() {
		return mGain;
	}

	float getDelay() {
		return mDelay;
	}

	float getCutoff() {
		return mCutoff;
	}

	float getTremolo() {
		return mTremolo;
	}

	bool getBypass() {
		return mBypass;
	}

	float getYZ1() {
		return mYZ1;
	}

	void process(const float * in, float * out, size_t nSamples) {
		//approx const float gain = powf(10.f, 0.05f * mGain);
		const float gain = ((2.6039890429412597e-4f * mGain + 0.032131027163547855f) * mGain + 1.f) / ((0.0012705124328080768f * mGain - 0.0666763481312185f) * mGain + 1.f);
		//approx const size_t delay = roundf(mSampleRate * 0.001f * mDelay);
		const size_t delay = (size_t)(mSampleRate * 0.001f * mDelay + 0.5f);
		const float mA1 = mSampleRate / (mSampleRate + 6.283185307179586f * mCutoff * mCutoffK);
		const float phaseInc = (1.f / 60.f) * (mSpeedK * mBPM) / mSampleRate;
		const float kt = 0.01f * mTremolo;
		for (size_t i = 0; i < nSamples; i++) {
			mDelayLine[mDelayLineCur] = in[i];
			const float x = mDelayLine[calcIndex(mDelayLineCur, delay, mDelayLineLength)];
			mDelayLineCur++;
			if (mDelayLineCur == mDelayLineLength)
				mDelayLineCur = 0;
			float y = x + mA1 * (mZ1 - x);
			mZ1 = y;
			mPhase += phaseInc;
			if (mPhase > 1.f)
				mPhase -= 1.f;
			const float lfo = mPhase + mPhase - 1.f;
			y *= 1.f + kt * lfo;
			out[i] = mBypass ? in[i] : gain * y;
			mYZ1 = out[i];
			mCount++;
		}
		int len = sprintf(mMsgBuf, "%.2f", mCount / mSampleRate); // possibly not RT-safe, for testing purposes only
		mMsgWrite(mHandle, len, mMsgBuf);
	}

	void resetCount() {
		mCount = 0;
	}

	void midiMsgIn(const uint8_t * data) {
		if (((data[0] & 0xf0) == 0x90) && (data[2] != 0))
			//approx mCutoffK = powf(2.f, (1.f / 12.f) * (note - 60));
			mCutoffK = data[1] < 64 ? (-0.19558034980097166f * data[1] - 2.361735109225749f) / (data[1] - 75.57552349522389f) : (393.95397927344214f - 7.660826245588588f * data[1]) / (data[1] - 139.0755234952239f);
	}

	void setPlaying(bool value) {
		mPlaying = value;
		updateSpeedK();
	}

	void setSpeed(float value) {
		mSpeed = value;
		updateSpeedK();
	}

	void setBPM(float value) {
		mBPM = value;
	}

	void setPhase(float value) {
		mPhase = value;
	}

	bool getPlaying() {
		return mPlaying;
	}

	float getSpeed() {
		return mSpeed;
	}

private:
	void updateSpeedK() {
		mSpeedK = mSpeed != 0.f ? mSpeed : (mPlaying ? 0.f : 1.f);
	}

	size_t calcIndex(size_t cur, size_t delay, size_t len) {
		return (cur < delay ? cur + len : cur) - delay;
	}

	float		mSampleRate;
	size_t		mDelayLineLength;

	float		mGain;
	float		mDelay;
	float		mCutoff;
	float		mTremolo;
	bool		mBypass;

	float *		mDelayLine;
	size_t		mDelayLineCur;
	float		mZ1;
	float		mCutoffK;
	bool		mPlaying;
	float		mSpeed;
	float		mSpeedK;
	float		mBPM;
	float		mPhase;
	float		mYZ1;

	uint64_t	mCount;
	char		mMsgBuf[1024];
	void *		mHandle;
	void (*mMsgWrite)(void *handle, size_t size, const void *data);
};

typedef struct {
	Plugin	p;
} plugin;

static void plugin_init(plugin *instance, const plugin_callbacks *cbs) {
	new(&instance->p) Plugin(cbs->handle, cbs->msg_write);
}

static void plugin_fini(plugin *instance) {
	(void)instance;
}

static void plugin_set_sample_rate(plugin *instance, float sample_rate) {
	instance->p.setSampleRate(sample_rate);
}

static size_t plugin_mem_req(plugin *instance) {
	return instance->p.memReq();
}

static void plugin_mem_set(plugin *instance, void *mem) {
	instance->p.memSet(mem);
}

static void plugin_reset(plugin *instance) {
	instance->p.reset();
}

static void plugin_set_parameter(plugin *instance, size_t index, float value) {
	switch (index) {
	case plugin_parameter_gain:
		instance->p.setGain(value);
		break;
	case plugin_parameter_delay:
		instance->p.setDelay(value);
		break;
	case plugin_parameter_cutoff:
		instance->p.setCutoff(value);
		break;
	case plugin_parameter_bypass:
		instance->p.setBypass(value >= 0.5f);
		break;
	}
}

static float plugin_get_parameter(plugin *instance, size_t index) {
	(void)index;
	return instance->p.getYZ1();
}

static void plugin_process(plugin *instance, const float **inputs, float **outputs, size_t n_samples) {
	instance->p.process(inputs[0], outputs[0], n_samples);
}

static void plugin_midi_msg_in(plugin *instance, size_t index, const uint8_t * data) {
	(void)index;
	instance->p.midiMsgIn(data);
}

static void plugin_msg_in(plugin *instance, size_t size, const void * data) {
	instance->p.resetCount();
	printf("%.*s\n", (int)size, (const char *)data); // yeah, not RT-safe, I know
}

static void plugin_set_transport(plugin *instance, const plugin_transport *transport) {
	if (transport->valid & PLUGIN_TRANSPORT_SPEED)
		instance->p.setSpeed(transport->speed);
	if (transport->valid & PLUGIN_TRANSPORT_PLAYING)
		instance->p.setPlaying(transport->playing);
	else if (transport->valid & PLUGIN_TRANSPORT_SPEED)
		instance->p.setPlaying(instance->p.getSpeed() != 0.f);

	if (transport->valid & PLUGIN_TRANSPORT_BPM)
		instance->p.setBPM(transport->bpm);

	if (instance->p.getPlaying()) {
		if (transport->valid & PLUGIN_TRANSPORT_QUARTER)
			instance->p.setPhase(transport->quarter - (uint32_t)transport->quarter);
		else if ((transport->valid & (PLUGIN_TRANSPORT_BEAT | PLUGIN_TRANSPORT_TIME_SIG_DENOM))
			== (PLUGIN_TRANSPORT_BEAT | PLUGIN_TRANSPORT_TIME_SIG_DENOM)) {
			// incorrect in case of time signature changes but that is the best LV2 supports
			double q = transport->beat * (4.0 / transport->time_sig_denom);
			instance->p.setPhase(q - (uint32_t)q);
		} else if ((transport->valid & (PLUGIN_TRANSPORT_TIME_SIG_NUM | PLUGIN_TRANSPORT_TIME_SIG_DENOM | PLUGIN_TRANSPORT_BAR | PLUGIN_TRANSPORT_BAR_BEAT))
			== (PLUGIN_TRANSPORT_TIME_SIG_NUM | PLUGIN_TRANSPORT_TIME_SIG_DENOM | PLUGIN_TRANSPORT_BAR | PLUGIN_TRANSPORT_BAR_BEAT)) {
			// even worse but that's the best we can do in Ardour/LV2
			double q = (transport->bar * transport->time_sig_num + transport->bar_beat) * (4.0 / transport->time_sig_denom);
			instance->p.setPhase(q - (uint32_t)q);
		}
	}
}

#ifdef PLUGIN_HAS_STATE

static void serialize_float(uint8_t *dest, float f) {
	union { float f; uint32_t u; } v;
	v.f = f;
	dest[0] = v.u & 0xff;
	dest[1] = (v.u & 0xff00) >> 8;
	dest[2] = (v.u & 0xff0000) >> 16;
	dest[3] = (v.u & 0xff000000) >> 24;
}

static float parse_float(const uint8_t *data) {
	union { float f; uint32_t u; } v;
	v.u = data[0];
	v.u |= data[1] << 8;
	v.u |= data[2] << 16;
	v.u |= data[3] << 24;
	return v.f;
}

static int plugin_state_save(plugin *instance, const plugin_state_callbacks *cbs, float last_sample_rate) {
	(void)last_sample_rate;
	uint8_t data[13];
	cbs->lock(cbs->handle);
	const float gain = instance->p.getGain();
	const float delay = instance->p.getDelay();
	const float cutoff = instance->p.getCutoff();
	const bool bypass = instance->p.getBypass();
	cbs->unlock(cbs->handle);
	serialize_float(data, gain);
	serialize_float(data + 4, delay);
	serialize_float(data + 8, cutoff);
	data[12] = bypass ? 1 : 0;
	return cbs->write(cbs->handle, (const char *)data, 13);
}

static char x_isnan(float x) {
	union { uint32_t u; float f; } v;
	v.f = x;
	return ((v.u & 0x7f800000) == 0x7f800000) && (v.u & 0x7fffff);
}

static int plugin_state_load(const plugin_state_callbacks *cbs, float cur_sample_rate, const char *data, size_t length) {
	(void)cur_sample_rate;
	if (length != 13)
		return -1;
	const uint8_t *d = (const uint8_t *)data;
	const float gain = parse_float(d);
	const float delay = parse_float(d + 4);
	const float cutoff = parse_float(d + 8);
	const float bypass = d[12] ? 1.f : 0.f;
	if (x_isnan(gain) || x_isnan(delay) || x_isnan(cutoff))
		return -1;
	cbs->lock(cbs->handle);
	cbs->set_parameter(cbs->handle, plugin_parameter_gain, gain);
	cbs->set_parameter(cbs->handle, plugin_parameter_delay, delay);
	cbs->set_parameter(cbs->handle, plugin_parameter_cutoff, cutoff);
	cbs->set_parameter(cbs->handle, plugin_parameter_bypass, bypass);
	cbs->unlock(cbs->handle);
	return 0;
}

#endif
