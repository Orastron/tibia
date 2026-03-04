#
# Tibia
#
# Copyright (C) 2023-2025 Orastron Srl unipersonale
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

TEMPLATE := web

include vars.mk

COMMON_DIR ?= .
DATA_DIR ?= .
PLUGIN_DIR ?= src
API_DIR ?= $(PLUGIN_DIR)
MKINC_DIR ?= $(COMMON_DIR)

BUILD_BIN_DIR := build/web
BUILD_DATA_DIR := build/web

-include $(MKINC_DIR)/vars-pre.mk

CC := clang
CXX := clang++
#already stripeped
#STRIP := strip

C_SRCS := $(COMMON_DIR)/src/walloc.c $(COMMON_DIR)/src/string.c $(C_SRCS_EXTRA)
CXX_SRCS := $(CXX_SRCS_EXTRA)
ifeq ($(wildcard $(PLUGIN_DIR)/plugin_cxx.h),)
	C_SRCS := $(COMMON_DIR)/src/processor.c $(C_SRCS)
else
	CXX_SRCS := build/src/processor.cpp $(CXX_SRCS)
	CXXFLAGS_EXTRA := -DHAS_PLUGIN_CXX_H $(CXXFLAGS_EXTRA)
endif
ifneq ($(CXX_SRCS),)
	CXX_SRCS := $(COMMON_DIR)/src/new.cpp $(CXX_SRCS)
endif

C_OBJS := $(addprefix build/obj/, $(notdir $(C_SRCS:.c=.o)))
CXX_OBJS := $(addprefix build/obj/, $(notdir $(CXX_SRCS:.cpp=.o)))

LIBS := $(LIBS_EXTRA)

CFLAGS := -O3 -ffast-math -Wall -Wpedantic -Wextra
CFLAGS_ALL := -I$(COMMON_DIR)/src -I$(DATA_DIR)/src -I$(PLUGIN_DIR) -I$(API_DIR) --target=wasm32 -flto -fvisibility=hidden $(CFLAGS_EXTRA) $(CFLAGS)

LDFLAGS_ALL := \
	-Wl,--allow-undefined \
	-Wl,--no-entry \
	-Wl,--lto-O3 \
	-Wl,-strip-all \
	-Wl,--export-table \
	-Wl,--export=malloc \
	-Wl,--export=realloc \
	-Wl,--export=calloc \
	-Wl,--export=free \
	-Wl,--export=memset \
	-Wl,--export=memcpy \
	-Wl,--export=processor_new \
	-Wl,--export=processor_free \
	-Wl,--export=processor_get_x_buf \
	-Wl,--export=processor_get_x \
	-Wl,--export=processor_get_zero_buf \
	-Wl,--export=processor_get_y_buf \
	-Wl,--export=processor_get_out_params \
	-Wl,--export=processor_process \
	-Wl,--export=processor_set_parameter \
	-Wl,-z,stack-size=$$((8*1024*1024)) \
	-nostdlib
ifeq ($(HAS_MIDI_IN), yes)
LDFLAGS_ALL := $(LDFLAGS_ALL) -Wl,--export=processor_midi_msg_in
endif
LDFLAGS_ALL := $(LDFLAGS_ALL) $(LDFLAGS_EXTRA) $(LDFLAGS)

CXXFLAGS := $(CFLAGS)
CXXFLAGS_ALL := -I$(COMMON_DIR)/src -I$(DATA_DIR)/src -I$(PLUGIN_DIR) -I$(API_DIR) --target=wasm32 -flto -fvisibility=hidden $(CXXFLAGS_EXTRA) $(CXXFLAGS)

DIRS := build build/obj build/web build/web/$(BUNDLE_NAME) build/src

ALL := build/web/$(BUNDLE_NAME)/module.wasm build/web/$(BUNDLE_NAME)/processor.js build/web/$(BUNDLE_NAME)/module.js

STRIP_ALL := build/web/$(BUNDLE_NAME)/module.wasm build/web/$(BUNDLE_NAME)/processor.js build/web/$(BUNDLE_NAME)/module.js
STRIP_PREREQS := $(STRIP_ALL)

PHONY := all clean strip

-include $(MKINC_DIR)/vars-extra.mk

all: $(ALL)

build/src/processor.cpp: $(COMMON_DIR)/src/processor.c | build/src
	cp $^ $@

ifeq ($(CXX_OBJS),)
build/web/$(BUNDLE_NAME)/module.wasm: $(C_OBJS) $(LIBS) | build/web/$(BUNDLE_NAME)
	$(CC) $^ -o $@ $(CFLAGS_ALL) $(LDFLAGS_ALL)
else
build/web/$(BUNDLE_NAME)/module.wasm: $(C_OBJS) $(CXX_OBJS) $(LIBS) | build/web/$(BUNDLE_NAME)
	$(CXX) $^ -o $@ $(CFLAGS_ALL) $(CXXFLAGS_ALL) $(LDFLAGS_ALL)
endif

build/web/$(BUNDLE_NAME)/processor.js: $(DATA_DIR)/src/processor.js | build/web/$(BUNDLE_NAME)
	cp $^ $@

build/web/$(BUNDLE_NAME)/module.js: $(DATA_DIR)/src/module.js | build/web/$(BUNDLE_NAME)
	cp $^ $@

$(DIRS):
	mkdir -p $@

clean:
	rm -fr build

strip: $(STRIP_PREREQS)
	#already stripped
	#wasm-strip build/web/$(BUNDLE_NAME)/module.wasm
	uglifyjs -c pure_funcs -m reserved build/web/$(BUNDLE_NAME)/module.js -o build/web/$(BUNDLE_NAME)/module.js
	uglifyjs -c pure_funcs -m reserved build/web/$(BUNDLE_NAME)/processor.js -o build/web/$(BUNDLE_NAME)/processor.js

-include $(MKINC_DIR)/rules-extra.mk

.PHONY: $(PHONY)

.SECONDEXPANSION:

PERCENT := %

$(C_OBJS): build/obj/%.o: $$(filter $$(PERCENT)/$$(basename $$(notdir $$@)).c,$$(C_SRCS)) | build/obj
	$(CC) $^ -o $@ -c $(CFLAGS_ALL)

$(CXX_OBJS): build/obj/%.o: $$(filter $$(PERCENT)/$$(basename $$(notdir $$@)).cpp,$$(CXX_SRCS)) | build/obj
	$(CXX) $^ -o $@ -c $(CXXFLAGS_ALL)

-include $(MKINC_DIR)/rules-secondexp-extra.mk

.DELETE_ON_ERROR:
