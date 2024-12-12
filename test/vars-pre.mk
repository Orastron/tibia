ifeq ($(TEMPLATE), cmd)
	TINYWAV_DIR := ../../../tinywav
	MIDI_PARSER_DIR := ../../../midi-parser
endif

ifeq ($(TEMPLATE), lv2)
	CFLAGS_EXTRA := $(shell pkg-config --cflags pugl-cairo-0 pugl-0 cairo)
	LDFLAGS_EXTRA := $(shell pkg-config --libs pugl-cairo-0 pugl-0 cairo) -Wl,-rpath,$(shell pkg-config --variable=libdir pugl-cairo-0),-rpath,$(shell pkg-config --variable=libdir pugl-0),-rpath,$(shell pkg-config --variable=libdir cairo)
endif

ifeq ($(TEMPLATE), vst3)
	CFLAGS_EXTRA := -I../../../vst3_c_api $(shell pkg-config --cflags pugl-cairo-0)
	LDFLAGS_EXTRA := $(shell pkg-config --libs pugl-cairo-0 pugl-0 cairo) -Wl,-rpath,$(shell pkg-config --variable=libdir pugl-cairo-0),-rpath,$(shell pkg-config --variable=libdir pugl-0),-rpath,$(shell pkg-config --variable=libdir cairo)
endif

ifeq ($(TEMPLATE), daisy-seed)
	LIBDAISY_DIR := ../../../libDaisy
endif

ifeq ($(TEMPLATE), android)
	CXXFLAGS_EXTRA := -I../../../miniaudio
	KEY_STORE := keystore.jks
	KEY_ALIAS := androidkey
	STORE_PASS := android
	KEY_PASS := android
	SDK_DIR := $(HOME)/Android/Sdk
	ANDROIDX_DIR := $(HOME)/Android/androidx
	KOTLIN_DIR := $(HOME)/Android/kotlin
	NDK_VERSION := 27.2.12479018
	BUILD_TOOLS_VERSION := 35.0.0
	ANDROID_VERSION := 35
	ANDROIDX_CORE_VERSION := 1.15.0
	ANDROIDX_LIFECYCLE_COMMON_VERSION := 2.8.7
	ANDROIDX_VERSIONEDPARCELABLE_VERSION := 1.2.0
	KOTLIN_STDLIB_VERSION := 2.1.0
	KOTLINX_COROUTINES_CORE_VERSION := 1.9.0
	KOTLINX_COROUTINES_CORE_JVM_VERSION := 1.9.0
endif
