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

SHELL := bash -o pipefail

TEMPLATE := cmd

include vars.mk

COMMON_DIR ?= .
DATA_DIR ?= .
PLUGIN_DIR ?= src
API_DIR ?= $(PLUGIN_DIR)
MKINC_DIR ?= $(COMMON_DIR)

BUILD_BIN_DIR := build
BUILD_DATA_DIR := build

-include $(MKINC_DIR)/vars-pre.mk

ifeq ($(OS), Windows_NT)
	EXE_SUFFIX := .exe
else
	UNAME_S := $(shell uname -s)
	EXE_SUFFIX :=
	PREFIX := /usr/local
	BINDIR := $(PREFIX)/bin
endif

TINYWAV_DIR ?= ../tinywav
MIDI_PARSER_DIR ?= ../midi-parser

CC := clang
CXX := clang++

C_SRCS := $(C_SRCS_EXTRA)
CXX_SRCS := $(CXX_SRCS_EXTRA)
ifeq ($(wildcard $(PLUGIN_DIR)/plugin_cxx.h),)
	C_SRCS := $(COMMON_DIR)/src/main.c $(C_SRCS)
else
	CXX_SRCS := build/src/main.cpp $(CXX_SRCS)
	CXXFLAGS_EXTRA := -DHAS_PLUGIN_CXX_H $(CXXFLAGS_EXTRA)
endif
ifeq ($(HAS_MIDI_IN), yes)
	C_SRCS := $(C_SRCS) $(MIDI_PARSER_DIR)/src/midi-parser.c
endif

C_OBJS := $(addprefix build/obj/, $(notdir $(C_SRCS:.c=.o)))
CXX_OBJS := $(addprefix build/obj/, $(notdir $(CXX_SRCS:.cpp=.o)))

CFLAGS := -O3 -Wall -Wpedantic -Wextra
CFLAGS_ALL := -I$(DATA_DIR)/src -I$(PLUGIN_DIR) -I$(API_DIR) -I$(TINYWAV_DIR) -I$(MIDI_PARSER_DIR)/include -fvisibility=hidden $(CFLAGS_EXTRA) $(CFLAGS)

LDFLAGS :=
LDFLAGS_ALL := $(LDFLAGS_EXTRA) $(LDFLAGS)

CXXFLAGS := $(CFLAGS)
CXXFLAGS_ALL := -I$(DATA_DIR)/src -I$(PLUGIN_DIR) -I$(API_DIR) -I$(TINYWAV_DIR) -I$(MIDI_PARSER_DIR)/include $(CXXFLAGS_EXTRA) $(CXXFLAGS)

ifeq ($(UNAME_S), Darwin)
	CFLAGS_ALL := $(CFLAGS_ALL) -arch arm64 -arch x86_64
	LDFLAGS_ALL := $(LDFLAGS_ALL) -arch arm64 -arch x86_64
	CXXFLAGS_ALL := $(CXXFLAGS_ALL) -arch arm64 -arch x86_64
endif
ifeq ($(OS), Windows_NT)
ifneq ($(CXX_OBJS),)
	LDFLAGS_ALL := -static-libstdc++ $(LDFLAGS_ALL)
endif
endif

PROGRAM := $(BUNDLE_NAME)$(EXE_SUFFIX)

DIRS := build build/obj build/src

ALL := build/$(PROGRAM)

STRIP_ALL := build/$(PROGRAM)
STRIP_PREREQS := $(STRIP_ALL)

PHONY := all clean strip
ifneq ($(OS), Windows_NT)
	PHONY := $(PHONY) install
endif

-include $(MKINC_DIR)/vars-extra.mk

all: $(ALL)

build/src/main.cpp: $(COMMON_DIR)/src/main.c | build/src
	cp $^ $@

ifeq ($(CXX_OBJS),)
build/$(PROGRAM): $(C_OBJS) build/obj/tinywav.o | build
	$(CC) $^ -o $@ $(CFLAGS_ALL) $(LDFLAGS_ALL)
else
build/$(PROGRAM): $(C_OBJS) $(CXX_OBJS) build/obj/tinywav.o | build
	$(CXX) $^ -o $@ $(CFLAGS_ALL) $(CXXFLAGS_ALL) $(LDFLAGS_ALL)
endif

$(DIRS):
	mkdir -p $@

clean:
	rm -fr build

strip: $(STRIP_PREREQS)
	strip build/$(PROGRAM)

ifneq ($(OS), Windows_NT)
install: all
	mkdir -m 0755 -p $(BINDIR)
	install -m 0755 build/$(PROGRAM) $(BINDIR)
endif

.PHONY: $(PHONY)

-include $(MKINC_DIR)/rules-extra.mk

.SECONDEXPANSION:

PERCENT := %

build/obj/tinywav.o: $(TINYWAV_DIR)/tinywav.c
	$(CC) $^ -o $@ -c $(CFLAGS_ALL) -Wno-unused-result

$(C_OBJS): build/obj/%.o: $$(filter $$(PERCENT)/$$(basename $$(notdir $$@)).c,$$(C_SRCS)) | build/obj
	$(CC) $^ -o $@ -c $(CFLAGS_ALL)

$(CXX_OBJS): build/obj/%.o: $$(filter $$(PERCENT)/$$(basename $$(notdir $$@)).cpp,$$(CXX_SRCS)) | build/obj
	$(CXX) $^ -o $@ -c $(CXXFLAGS_ALL)

-include $(MKINC_DIR)/rules-secondexp-extra.mk

.DELETE_ON_ERROR:
