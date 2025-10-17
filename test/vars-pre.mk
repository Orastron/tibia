#
# Tibia
#
# Copyright (C) 2024, 2025 Orastron Srl unipersonale
#
# Tibia is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3 of the License.
#
# Tibia is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Tibia.  If not, see <http://www.gnu.org/licenses/>.
#
# File author: Stefano D'Angelo
#

API_DIR := ../../api

VINCI_DIR := ../../../../vinci

ifeq ($(TEMPLATE), cmd)
	TINYWAV_DIR := ../../../../tinywav
	MIDI_PARSER_DIR := ../../../../midi-parser
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
	CFLAGS_EXTRA := $(CFLAGS_EXTRA) -I$(VINCI_DIR)
	CXXFLAGS_EXTRA := $(CXXFLAGS_EXTRA) -I$(VINCI_DIR)
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
	CFLAGS_EXTRA := $(CFLAGS_EXTRA) -I../../../../vst3_c_api -I$(VINCI_DIR)
	CXXFLAGS_EXTRA := $(CXXFLAGS_EXTRA) -I../../../../vst3_c_api -I$(VINCI_DIR)
endif

ifeq ($(TEMPLATE), daisy-seed)
	LIBDAISY_DIR := ../../../../libDaisy
endif

ifeq ($(TEMPLATE), android)
	CXXFLAGS_EXTRA := -I../../../../miniaudio
	KEY_STORE := keystore.jks
	KEY_ALIAS := androidkey
	STORE_PASS := android
	KEY_PASS := android
	SDK_DIR := $(HOME)/Android/Sdk
	ANDROIDX_DIR := $(HOME)/Android/androidx
	KOTLIN_DIR := $(HOME)/Android/kotlin
	NDK_VERSION := 29.0.14206865
	BUILD_TOOLS_VERSION := 36.1.0
	ANDROIDX_CORE_VERSION := 1.17.0
	ANDROIDX_LIFECYCLE_COMMON_VERSION := 2.9.4
	ANDROIDX_VERSIONEDPARCELABLE_VERSION := 1.2.1
	KOTLIN_STDLIB_VERSION := 2.2.20
	KOTLINX_COROUTINES_CORE_VERSION := 1.10.2
	KOTLINX_COROUTINES_CORE_JVM_VERSION := 1.10.2
endif
