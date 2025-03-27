# Tibia

Tibia is a template-based generator of sound generation/processing software in the form of applications and audio plugins. You supply it with a modest amount of JSON metadata and C code and it spits out an entire project with all the necessary boilerplate. Its purpose is to minimize the amount of code to be written and abstract away target APIs/platforms.

## Status

Right now Tibia contains templates to generate:

* VST3 plugins
* LV2 plugins
* Android apps
* iOS apps
* WebAudio modules, using WebAssembly
* Daisy Seed firmware
* command line applications, with .wav I/O and MIDI file format 0 input

At the moment this software should be considered unstable and undocumented. We currently have no plans to make this tool actually something to be relied upon, but this might change in the future.

Feel free to try it out anyway and perhaps give us some feedback (we'd appreciate that). If you need something more stable and structured right now, you better try [JUCE](https://juce.com/), [iPlug2](https://iplug2.github.io/), [DPF](https://github.com/DISTRHO/DPF), [Jamba](https://jamba.dev/), etc.

## Legal

Copyright (C) 2021-2025 Orastron Srl unipersonale.

Authors: Stefano D'Angelo, Paolo Marrone.

All the code in the repo is released under GPLv3. See the LICENSE file.

VST is a registered trademark of Steinberg Media Technologies GmbH.

All trademarks and registered marks are properties of their respective owners. All company, product, and service names used are for identification purposes only. Use of these names, trademarks, and brands does not imply endorsement.
