API_DIR := ../api

ifeq ($(TEMPLATE), cmd)
	TINYWAV_DIR := ../../../tinywav
	MIDI_PARSER_DIR := ../../../midi-parser
endif

ifeq ($(TEMPLATE), lv2)
	ifeq ($(OS), Windows_NT)
	else
		UNAME_S := $(shell uname -s)
		ifeq ($(UNAME_S), Darwin)
			M_SRCS_EXTRA := $(PLUGIN_DIR)/ui_apple.m
			LDFLAGS_EXTRA := -framework Cocoa -framework WebKit
		else
			CFLAGS_EXTRA := $(shell pkg-config --cflags x11)
			LDFLAGS_EXTRA := $(shell pkg-config --libs x11) -Wl,-rpath,$(shell pkg-config --variable=libdir x11)
		endif
	endif
endif

ifeq ($(TEMPLATE), vst3)
	ifeq ($(OS), Windows_NT)
	else
		UNAME_S := $(shell uname -s)
		ifeq ($(UNAME_S), Darwin)
			M_SRCS_EXTRA := $(PLUGIN_DIR)/ui_apple.m
			LDFLAGS_EXTRA := -framework Cocoa -framework WebKit
		else
			CFLAGS_EXTRA := $(shell pkg-config --cflags x11)
			LDFLAGS_EXTRA := $(shell pkg-config --libs x11) -Wl,-rpath,$(shell pkg-config --variable=libdir x11)
		endif
	endif
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
	NDK_VERSION := 28.0.13004108
	BUILD_TOOLS_VERSION := 36.0.0
	ANDROID_VERSION := 36
	ANDROIDX_CORE_VERSION := 1.15.0
	ANDROIDX_LIFECYCLE_COMMON_VERSION := 2.8.7
	ANDROIDX_VERSIONEDPARCELABLE_VERSION := 1.2.1
	KOTLIN_STDLIB_VERSION := 2.1.10
	KOTLINX_COROUTINES_CORE_VERSION := 1.10.1
	KOTLINX_COROUTINES_CORE_JVM_VERSION := 1.10.1
endif
