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

TEMPLATE := vst3

include vars.mk

COMMON_DIR ?= .
DATA_DIR ?= .
PLUGIN_DIR ?= src
API_DIR ?= $(PLUGIN_DIR)
MKINC_DIR ?= $(COMMON_DIR)

BUNDLE_DIR := $(BUNDLE_NAME).vst3

ifeq ($(OS), Windows_NT)
	DLL_SUFFIX := .vst3
	VST3DIR := $(subst \,/,$(COMMONPROGRAMFILES))/VST3
	VST3DIR_USER := $(subst \,/,$(LOCALAPPDATA))/Programs/Common/VST3
	CC := gcc
	CXX := g++
	MACHINE := $(shell $(CC) -dumpmachine | sed 's:-.*::g')
	ifeq ($(MACHINE), x86_64)
		VST3_PLATFORM := x86_64-win
	endif
	ifeq ($(MACHINE), aarch64)
		VST3_PLATFORM := arm64-win
	endif
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S), Darwin)
		DLL_SUFFIX :=
		VST3DIR := /Library/Audio/Plug-Ins/VST3
		VST3DIR_USER := $(HOME)/Library/Audio/Plug-Ins/VST3
		CC := clang
		CXX := clang++
		VST3_PLATFORM := MacOS
	else
		DLL_SUFFIX := .so
		VST3DIR := /usr/local/lib/vst3
		VST3DIR_USER := $(HOME)/.vst3
		CC := gcc
		CXX := g++
		VST3_PLATFORM := $(shell uname -m)-linux
	endif
endif
DLL_DIR := Contents/$(VST3_PLATFORM)

BUILD_BIN_DIR := build/$(BUNDLE_DIR)/$(DLL_DIR)
BUILD_DATA_DIR :=  build/$(BUNDLE_DIR)/Contents/Resources

-include $(MKINC_DIR)/vars-pre.mk

CFLAGS := -O3 -std=c11 -Wall -Wpedantic -Wextra
CFLAGS_ALL := -I$(DATA_DIR)/src -I$(PLUGIN_DIR) -I$(API_DIR) -fPIC -fvisibility=hidden $(CFLAGS_EXTRA) $(CFLAGS)

LDFLAGS :=
LDFLAGS_ALL := -shared -lm $(LDFLAGS_EXTRA) $(LDFLAGS)

CXXFLAGS := $(CFLAGS)
CXXFLAGS_ALL := -I$(DATA_DIR)/src -I$(PLUGIN_DIR) -I$(API_DIR) -fPIC $(CXXFLAGS_EXTRA) $(CXXFLAGS)

ifeq ($(UNAME_S), Darwin)
	CFLAGS_ALL := $(CFLAGS_ALL) -arch arm64 -arch x86_64
	LDFLAGS_ALL := $(LDFLAGS_ALL) -arch arm64 -arch x86_64
	CXXFLAGS_ALL := $(CXXFLAGS_ALL) -arch arm64 -arch x86_64
	ifeq ($(HAS_UI), yes)
		LDFLAGS_ALL := $(LDFLAGS_ALL) -Wl,-framework,Foundation -Wl,-framework,Cocoa -Wl,-framework,Corevideo
	endif
else
	LDFLAGS_ALL := -static $(LDFLAGS_ALL)
endif

ifeq ($(UNAME_S), Linux)
	CFLAGS_ALL := $(CFLAGS_ALL) -D_GNU_SOURCE
	ifeq ($(HAS_UI), yes)
		CFLAGS_ALL := $(CFLAGS_ALL) $(shell pkg-config --cflags x11)
		LDFLAGS_ALL := $(LDFLAGS_ALL) $(shell pkg-config --libs x11)
		CXXFLAGS_ALL := $(CXXFLAGS_ALL) $(shell pkg-config --cflags x11)
	endif
endif

DLL_FILE := $(DLL_DIR)/$(BUNDLE_NAME)$(DLL_SUFFIX)

C_SRCS := $(COMMON_DIR)/src/vst3.c $(COMMON_DIR)/src/tinycthread.c $(C_SRCS_EXTRA)
C_OBJS := $(addprefix build/obj/, $(notdir $(C_SRCS:.c=.o)))

M_SRCS := $(M_SRCS_EXTRA)
M_OBJS := $(addprefix build/obj/, $(notdir $(M_SRCS:.m=.o)))

CXX_SRCS := $(CXX_SRCS_EXTRA)
CXX_OBJS := $(addprefix build/obj/, $(notdir $(CXX_SRCS:.cpp=.o)))

DIRS := build build/obj build/$(BUNDLE_DIR) build/$(BUNDLE_DIR)/Contents build/$(BUNDLE_DIR)/Contents/Resources build/$(BUNDLE_DIR)/$(DLL_DIR)

ALL := build/$(BUNDLE_DIR)/$(DLL_FILE)
ifeq ($(UNAME_S), Darwin)
	ALL := $(ALL) build/$(BUNDLE_DIR)/Contents/Info.plist build/$(BUNDLE_DIR)/Contents/PkgInfo
endif

STRIP_ALL := build/$(BUNDLE_DIR)/$(DLL_FILE)
STRIP_PREREQS := $(STRIP_ALL)

PHONY := all clean strip install install-user

-include $(MKINC_DIR)/vars-extra.mk

all: $(ALL)

ifeq ($(CXX_OBJS),)
build/$(BUNDLE_DIR)/$(DLL_FILE): $(C_OBJS) $(M_OBJS) | build/$(BUNDLE_DIR)/$(DLL_DIR)
	$(CC) $^ -o $@ $(CFLAGS_ALL) $(LDFLAGS_ALL)
else
build/$(BUNDLE_DIR)/$(DLL_FILE): $(C_OBJS) $(M_OBJS) $(CXX_OBJS) | build/$(BUNDLE_DIR)/$(DLL_DIR)
	$(CXX) $^ -o $@ $(CFLAGS_ALL) $(CXXFLAGS_ALL) $(LDFLAGS_ALL)
endif

ifeq ($(UNAME_S), Darwin)
build/$(BUNDLE_DIR)/Contents/%: $(DATA_DIR)/data/% | build/$(BUNDLE_DIR)/Contents
	cp $^ $@

build/$(BUNDLE_DIR)/Contents/%: $(COMMON_DIR)/data/% | build/$(BUNDLE_DIR)/Contents
	cp $^ $@
endif

$(DIRS):
	mkdir -p $@

clean:
	rm -fr build

strip: $(STRIP_PREREQS)
	strip -Sx build/$(BUNDLE_DIR)/$(DLL_FILE)

install: all
	@for d in `find build/$(BUNDLE_DIR) -type d`; do \
		d=`echo $$d | sed 's:^build/::'` ; \
		echo mkdir -m 0755 -p "$(VST3DIR)/$$d"; \
		mkdir -m 0755 -p "$(VST3DIR)/$$d" || exit 1; \
	done
	@for f in `find build/$(BUNDLE_DIR) -type f`; do \
		m=`[ -x $$f ] && echo 0755 || echo 0644`; \
		d=`echo $$f | sed 's:^build/::'` ; \
		d=`dirname $$d`; \
		echo install -m $$m $$f "$(VST3DIR)/$$d"; \
		install -m $$m $$f "$(VST3DIR)/$$d" || exit 1; \
	done

install-user: all
	@for d in `find build/$(BUNDLE_DIR) -type d`; do \
		d=`echo $$d | sed 's:^build/::'` ; \
		echo mkdir -m 0755 -p "$(VST3DIR_USER)/$$d"; \
		mkdir -m 0755 -p "$(VST3DIR_USER)/$$d" || exit 1; \
	done
	@for f in `find build/$(BUNDLE_DIR) -type f`; do \
		m=`[ -x $$f ] && echo 0755 || echo 0644`; \
		d=`echo $$f | sed 's:^build/::'` ; \
		d=`dirname $$d`; \
		echo install -m $$m $$f "$(VST3DIR_USER)/$$d"; \
		install -m $$m $$f "$(VST3DIR_USER)/$$d" || exit 1; \
	done

-include $(MKINC_DIR)/rules-extra.mk

.PHONY: $(PHONY)

.SECONDEXPANSION:

PERCENT := %

$(C_OBJS): build/obj/%.o: $$(filter $$(PERCENT)/$$(basename $$(notdir $$@)).c,$$(C_SRCS)) | build/obj
	$(CC) $^ -o $@ -c $(CFLAGS_ALL)

$(M_OBJS): build/obj/%.o: $$(filter $$(PERCENT)/$$(basename $$(notdir $$@)).m,$$(M_SRCS)) | build/obj
	$(CC) $^ -o $@ -c $(CFLAGS_ALL)

$(CXX_OBJS): build/obj/%.o: $$(filter $$(PERCENT)/$$(basename $$(notdir $$@)).cpp,$$(CXX_SRCS)) | build/obj
	$(CXX) $^ -o $@ -c $(CXXFLAGS_ALL)

-include $(MKINC_DIR)/rules-secondexp-extra.mk

.DELETE_ON_ERROR:
