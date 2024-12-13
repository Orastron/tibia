#
# Tibia
#
# Copyright (C) 2024 Orastron Srl unipersonale
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

TEMPLATE := lv2

include vars.mk

COMMON_DIR ?= .
DATA_DIR ?= .
PLUGIN_DIR ?= src
API_DIR ?= $(PLUGIN_DIR)
MKINC_DIR ?= $(COMMON_DIR)

-include $(MKINC_DIR)/vars-pre.mk

ifeq ($(OS), Windows_NT)
	DLL_SUFFIX := .dll
	UI_TYPE := WindowsUI
	LV2DIR := $(shell echo '$(COMMONPROGRAMFILES)' | sed 's:\\:/:g')/LV2
	LV2DIR_USER := $(shell echo '$(APPDATA)' | sed 's:\\:/:g')/LV2
	CC := gcc
	CXX := g++
else
	UNAME_S = $(shell uname -s)
	ifeq ($(UNAME_S), Darwin)
		DLL_SUFFIX := .dylib
		UI_TYPE := CocoaUI
		LV2DIR := /Library/Audio/Plug-Ins/LV2
		LV2DIR_USER := $(HOME)/Library/Audio/Plug-Ins/LV2
		CC := clang
		CXX := clang++
	else
		DLL_SUFFIX := .so
		UI_TYPE := X11UI
		PREFIX := /usr/local
		LV2DIR := $(PREFIX)/lib/lv2
		LV2DIR_USER := $(HOME)/.lv2
		CC := gcc
		CXX := g++
	endif
endif

CFLAGS := -O3 -Wall -Wpedantic -Wextra
CFLAGS_ALL := -I$(DATA_DIR)/src -I$(PLUGIN_DIR) -I$(API_DIR) $(shell pkg-config --cflags lv2) -fPIC $(CFLAGS_EXTRA) $(CFLAGS)

LDFLAGS :=
LDFLAGS_ALL := -shared $(shell pkg-config --libs lv2) $(LDFLAGS_EXTRA) $(LDFLAGS)

CXXFLAGS := $(CFLAGS)
CXXFLAGS_ALL := -I$(DATA_DIR)/src -I$(PLUGIN_DIR) -I$(API_DIR) -fPIC $(CXXFLAGS_EXTRA) $(CXXFLAGS)

ifeq ($(UNAME_S), Darwin)
	CFLAGS_ALL := $(CFLAGS_ALL) -arch arm64 -arch x86_64
	LDFLAGS_ALL := $(LDFLAGS_ALL) -arch arm64 -arch x86_64
	CXXFLAGS_ALL := $(CXXFLAGS_ALL) -arch arm64 -arch x86_64
endif

BUNDLE_DIR := $(BUNDLE_NAME).lv2

DLL_FILE := $(BUNDLE_NAME)$(DLL_SUFFIX)

C_SRCS := $(COMMON_DIR)/src/lv2.c $(C_SRCS_EXTRA)
C_OBJS := $(addprefix build/obj/, $(notdir $(C_SRCS:.c=.o)))

CXX_SRCS := $(CXX_SRCS_EXTRA)
CXX_OBJS := $(addprefix build/obj/, $(notdir $(CXX_SRCS:.cpp=.o)))

ALL := build/$(BUNDLE_DIR)/manifest.ttl build/$(BUNDLE_DIR)/$(DLL_FILE)

BUILD_BIN_DIR := build/$(BUNDLE_DIR)
BUILD_DATA_DIR := build/$(BUNDLE_DIR)

-include $(MKINC_DIR)/vars-extra.mk

all: $(ALL)

build/$(BUNDLE_DIR)/manifest.ttl: $(DATA_DIR)/data/manifest.ttl.in | build/$(BUNDLE_DIR)
	cat $^ | sed s/@DLL_SUFFIX@/$(DLL_SUFFIX)/g | sed s/@UI_TYPE@/$(UI_TYPE)/g > $@

ifeq ($(CXX_OBJS),)
build/$(BUNDLE_DIR)/$(DLL_FILE): $(C_OBJS) | build/$(BUNDLE_DIR)
	$(CC) $^ -o $@ $(CFLAGS_ALL) $(LDFLAGS_ALL)
else
build/$(BUNDLE_DIR)/$(DLL_FILE): $(C_OBJS) $(CXX_OBJS) | build/$(BUNDLE_DIR)
	$(CXX) $^ -o $@ $(CFLAGS_ALL) $(CXXFLAGS_ALL) $(LDFLAGS_ALL)
endif

build/$(BUNDLE_DIR) build/obj:
	mkdir -p $@

clean:
	rm -fr build

install: all
	@for d in `find build/$(BUNDLE_DIR) -type d`; do \
		d=`echo $$d | sed 's:^build/::'` ; \
		echo mkdir -m 0755 -p "$(LV2DIR)/$$d"; \
		mkdir -m 0755 -p "$(LV2DIR)/$$d"; \
	done
	@for f in `find build/$(BUNDLE_DIR) -type f`; do \
		m=`[ -x $$f ] && echo 0755 || echo 0644`; \
		d=`echo $$f | sed 's:^build/::'` ; \
		d=`dirname $$d`; \
		echo install -m $$m $$f "$(LV2DIR)/$$d"; \
		install -m $$m $$f "$(LV2DIR)/$$d"; \
	done

install-user: all
	@for d in `find build/$(BUNDLE_DIR) -type d`; do \
		d=`echo $$d | sed 's:^build/::'` ; \
		echo mkdir -m 0755 -p "$(LV2DIR_USER)/$$d"; \
		mkdir -m 0755 -p "$(LV2DIR_USER)/$$d"; \
	done
	@for f in `find build/$(BUNDLE_DIR) -type f`; do \
		m=`[ -x $$f ] && echo 0755 || echo 0644`; \
		d=`echo $$f | sed 's:^build/::'` ; \
		d=`dirname $$d`; \
		echo install -m $$m $$f "$(LV2DIR_USER)/$$d"; \
		install -m $$m $$f "$(LV2DIR_USER)/$$d"; \
	done

-include $(MKINC_DIR)/rules-extra.mk

.PHONY: all clean install install-user

.SECONDEXPANSION:

PERCENT := %

$(C_OBJS): build/obj/%.o: $$(filter $$(PERCENT)/$$(basename $$(notdir $$@)).c,$$(C_SRCS)) | build/obj
	$(CC) $^ -o $@ -c $(CFLAGS_ALL)

$(CXX_OBJS): build/obj/%.o: $$(filter $$(PERCENT)/$$(basename $$(notdir $$@)).cpp,$$(CXX_SRCS)) | build/obj
	$(CXX) $^ -o $@ -c $(CXXFLAGS_ALL)

-include $(MKINC_DIR)/rules-secondexp-extra.mk
