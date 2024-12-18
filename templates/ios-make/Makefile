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

TEMPLATE := ios

include vars.mk

COMMON_DIR ?= .
DATA_DIR ?= .
PLUGIN_DIR ?= src
API_DIR ?= $(PLUGIN_DIR)
MKINC_DIR ?= $(COMMON_DIR)

BUILD_BIN_DIR  := build/gen/res
BUILD_DATA_DIR := build/gen/res

-include $(MKINC_DIR)/vars-pre.mk

SOURCES := \
	$(DATA_DIR)/src/data.h \
	$(COMMON_DIR)/src/app.swift \
	$(COMMON_DIR)/src/native.mm \
	$(COMMON_DIR)/src/app-Bridging-Header.h \
	$(PLUGIN_DIR)/plugin.h \
	$(API_DIR)/plugin_api.h \
	$(C_SRCS_EXTRA) \
	$(CXX_SRCS_EXTRA) \
	$(SRCS_EXTRA)
SOURCES_OUT := $(addprefix build/gen/src/, $(notdir $(SOURCES)))

RESOURCES     := $(DATA_DIR)/src/index.html
RESOURCES_OUT := $(addprefix build/gen/res/, $(notdir $(RESOURCES)))

DIRS := build build/gen build/gen/src build/gen/res

ALL := build/gen/$(BUNDLE_NAME).xcodeproj

-include $(MKINC_DIR)/vars-extra.mk

all: $(ALL)

build/gen/$(BUNDLE_NAME).xcodeproj: $(SOURCES_OUT) $(RESOURCES_OUT)
	xcodegen generate --spec project.yml -r build/gen -p build/gen

$(DIRS):
	mkdir -p $@

clean:
	rm -fr build

-include $(MKINC_DIR)/rules-extra.mk

.PHONY: all clean

.SECONDEXPANSION:

PERCENT := %

$(SOURCES_OUT): build/gen/src/%: $$(filter $$(PERCENT)/%,$$(SOURCES)) | build/gen/src
	cp -R $^ $@

$(RESOURCES_OUT): build/gen/res/%: $$(filter $$(PERCENT)/%,$$(RESOURCES)) | build/gen/res
	cp -R $^ $@

-include $(MKINC_DIR)/rules-secondexp-extra.mk
