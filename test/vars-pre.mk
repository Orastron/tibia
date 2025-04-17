API_DIR := ../api

VINCI_DIR := ../../../vinci

ifeq ($(TEMPLATE), cmd)
	TINYWAV_DIR := ../../../tinywav
	MIDI_PARSER_DIR := ../../../midi-parser
endif

ifeq ($(TEMPLATE), lv2)
	ifeq ($(OS), Windows_NT)
		C_SRCS_EXTRA  := $(VINCI_DIR)/vinci-win32.c
		LDFLAGS_EXTRA := -mwindows
	else
		UNAME_S := $(shell uname -s)
		ifeq ($(UNAME_S), Darwin)
			M_SRCS_EXTRA  := $(VINCI_DIR)/vinci-cocoa.m
			LDFLAGS_EXTRA := -framework Cocoa -lobjc
		else
			C_SRCS_EXTRA  := $(VINCI_DIR)/vinci-xcb.c
			LDFLAGS_EXTRA := -lxcb
		endif
	endif
	CFLAGS_EXTRA := $(CFLAGS_EXTRA) -I${VINCI_DIR}
endif

ifeq ($(TEMPLATE), vst3)
	ifeq ($(OS), Windows_NT)
		C_SRCS_EXTRA  := $(VINCI_DIR)/vinci-win32.c
		LDFLAGS_EXTRA := -mwindows
	else
		UNAME_S := $(shell uname -s)
		ifeq ($(UNAME_S), Darwin)
			M_SRCS_EXTRA  := $(VINCI_DIR)/vinci-cocoa.m
			LDFLAGS_EXTRA := -framework Cocoa -lobjc
		else
			C_SRCS_EXTRA  := $(VINCI_DIR)/vinci-xcb.c
			LDFLAGS_EXTRA := -lxcb
		endif
	endif
	CFLAGS_EXTRA := $(CFLAGS_EXTRA)	-I../../../vst3_c_api
	CFLAGS_EXTRA := $(CFLAGS_EXTRA) -I${VINCI_DIR}
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
	ANDROIDX_CORE_VERSION := 1.16.0
	ANDROIDX_LIFECYCLE_COMMON_VERSION := 2.8.7
	ANDROIDX_VERSIONEDPARCELABLE_VERSION := 1.2.1
	KOTLIN_STDLIB_VERSION := 2.1.20
	KOTLINX_COROUTINES_CORE_VERSION := 1.10.2
	KOTLINX_COROUTINES_CORE_JVM_VERSION := 1.10.2
endif
