/*
 * Tibia
 *
 * Copyright (C) 2022, 2024 Orastron Srl unipersonale
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

var buses      = {{=JSON.stringify(it.product.buses, null, 2)}};
var parameters = {{=JSON.stringify(it.product.parameters, null, 2)}};

var busesIn    = buses.filter(x => x.type == "audio" && x.direction == "input");
var busesOut   = buses.filter(x => x.type == "audio" && x.direction == "output");

var nChansIn   = busesIn.reduce((a, x) => a + (x.channels == "mono" ? 1 : 2), 0);
var nChansOut  = busesOut.reduce((a, x) => a + (x.channels == "mono" ? 1 : 2), 0);

var cpu_meter  = 0.0;
var sampleRate = 1.0;

class Processor extends AudioWorkletProcessor {
	constructor(options) {
		super();

		var module = new WebAssembly.Module(options.processorOptions.wasmBytes);
		var instance = new WebAssembly.Instance(module, { env: {} });
		this.module = instance.exports;

		this.instance = this.module.processor_new(sampleRate);
		if (!this.instance)
			throw "Could not instantiate processor module";

		if (nChansIn > 0) {
			this.xBufP = this.module.processor_get_x_buf(this.instance);
			this.zeroBufP = this.module.processor_get_zero_buf(this.instance);
			this.xBuf = new Float32Array(this.module.memory.buffer, this.xBufP, 128 * nChansIn);
			this.x = new Uint32Array(this.module.memory.buffer, this.module.processor_get_x(this.instance), nChansIn);
		}
		if (nChansOut > 0)
			this.yBuf = new Float32Array(this.module.memory.buffer, this.module.processor_get_y_buf(this.instance), 128 * nChansOut);

		this.parametersIn = [];
		for (var i = 0; i < parameters.length; i++)
			if (parameters[i].direction == "input")
				this.parametersIn.push({ index: i, value: NaN });

		this.parametersOut = [];
		for (var i = 0; i < parameters.length; i++)
			if (parameters[i].direction == "output")
				this.parametersOut.push({ index: i, value: NaN });

		if (this.parametersOut.length > 0)
			this.parametersOutValues = new Float32Array(this.module.memory.buffer, this.module.processor_get_out_params(this.instance), this.parametersOut.length);
		
		this.paramOutChangeMsg = { type: "paramOutChange", index: NaN, value: NaN };

		var self = this;
		this.port.onmessage = function (e) {
			if (e.data.type == "midi")
				self.module.processor_midi_msg_in(self.instance, e.data.index, e.data.data[0], e.data.data[1], e.data.data[2]);
		};
	}

	process(inputs, outputs, params) {
		const processTimeStart = performance.now();

		for (var i = 0; i < this.parametersIn.length; i++) {
			var index = this.parametersIn[i].index;
			var parameter = parameters[index];
			var name = parameter.name;
			var value = params[name][0];
			if (value != this.parametersIn[i].value) {
				if (parameter.isBypass || parameter.toggled)
					value = value > 0 ? 1 : 0;
				else if (parameter.integer)
					value = Math.round(value);
				value = Math.min(Math.max(value, parameter.minimum), parameter.maximum);
				if (value != this.parametersIn[i].value) {
					this.module.processor_set_parameter(this.instance, index, value);
					this.parametersIn[i].value = value;
				}
			}
		}

		var n = outputs[0][0].length;
		var i = 0;
		while (i < n) {
			var s = Math.min(n - i, 128);

			var j = 0;
			var o = 0;
			for (var k = 0; k < busesIn.length; k++) {
				var bus = busesIn[k];
				var input = inputs[k];

				if (!input[0])
					this.x[j] = bus.optional ? 0 : this.zeroBufP;
				else {
					this.xBuf.set(input[0].subarray(i, i + s), o);
					this.x[j] = this.xBufP + 4 * o;
					o += 128;
				}
				j++;

				if (bus.channels == "stereo") {
					if (!input[0])
						this.x[j] = this.x[j - 1];
					else if (!input[1])
						this.x[j] = this.zeroBufP;
					else {
						this.xBuf.set(input[1].subarray(i, i + s), o);
						this.x[j] = this.xBufP + 4 * o;
						o += 128;
					}
					j++;
				}
			}

			this.module.processor_process(this.instance, s);

			var j = 0;
			for (var k = 0; k < outputs.length; k++) {
				var output = outputs[k];
				output[0].set(this.yBuf.subarray(128 * j, 128 * j + s), i);
				j++;
				if (output[1]) {
					output[1].set(this.yBuf.subarray(128 * j, 128 * j + s), i);
					j++;
				}
			}

			i += s;
		}
		
		for (var i = 0; i < this.parametersOut.length; i++) {
			var index = this.parametersOut[i].index;
			var value;
			if (parameters[index].isCpumeter)
				value = cpu_meter;
			else
				value = this.parametersOutValues[i];
			if (value != this.parametersOut[i].value) {
				this.paramOutChangeMsg.index = index;
				this.paramOutChangeMsg.value = value;
				this.port.postMessage(this.paramOutChangeMsg);
				this.parametersOut[i].value = value;
			}
		}
		const processTimeEnd = performance.now();
		const processTimeMs  = processTimeEnd - processTimeStart;
		const processTimeS   = processTimeMs * 0.001;
		cpu_meter = cpu_meter * 0.9 + (processTimeS * sampleRate) * 0.1;

		return true; // because Chrome sucks: https://bugs.chromium.org/p/chromium/issues/detail?id=921354
	}

	static get parameterDescriptors() {
		var ret = [];
		for (var i = 0; i < parameters.length; i++) {
			var p = parameters[i];
			if (p.direction == "output")
				continue;
			ret.push({ name: p.name, minValue: p.minimum, maxValue: p.maximum, defaultValue: p.defaultValue, automationRate: "k-rate" });
		}
		return ret;
	}
}

registerProcessor("{{=it.product.bundleName}}", Processor);
