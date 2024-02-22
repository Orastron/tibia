/*
 * Tibia
 *
 * Copyright (C) 2022-2024 Orastron Srl unipersonale
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

const data = {
	company: {{=JSON.stringify(it.company, null, 2)}},
	product: {{=JSON.stringify(it.product, null, 2)}}
};

export class Module {
	static get data() { return data; }

	async init(context, processorJsPath, wasmPath) {
		var wasmBytes = await fetch(wasmPath)
					.then(response => response.arrayBuffer())
					.then(bytes => bytes);
		if (!wasmBytes.byteLength)
			throw "Could not fetch WebAssembly module";

		await context.audioWorklet.addModule(processorJsPath);

		this.context = context;
		this.wasmBytes = wasmBytes;
	}

	context = null;
	wasmBytes = null;
}

export class Node extends AudioWorkletNode {
	constructor(module) {
		super(module.context, data.product.bundleName,
			{
				numberOfInputs: data.product.buses.filter(b => b.type == "audio" && b.direction == "input").length,
				numberOfOutputs: data.product.buses.filter(b => b.type == "audio" && b.direction == "output").length,
				outputChannelCount: data.product.buses.filter(b => b.type == "audio" && b.direction == "output").map(b => b.channels == "mono" ? 1 : 2),
				processorOptions: { wasmBytes: module.wasmBytes }
			});
	}
}
