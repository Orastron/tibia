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

TEMPLATE := daisy-seed

include vars.mk

COMMON_DIR ?= .
DATA_DIR ?= .
PLUGIN_DIR ?= src
API_DIR ?= $(PLUGIN_DIR)
MKINC_DIR ?= $(COMMON_DIR)

-include $(MKINC_DIR)/vars-pre.mk

LIBDAISY_DIR ?= ../libDaisy

TARGET := $(BUNDLE_NAME)

CPP_SOURCES := $(COMMON_DIR)/src/main.cpp $(CXX_SRCS_EXTRA)

SYSTEM_FILES_DIR := $(LIBDAISY_DIR)/core

include $(SYSTEM_FILES_DIR)/Makefile

C_SOURCES += $(C_SRCS_EXTRA)

CFLAGS += -I$(DATA_DIR)/src -I$(PLUGIN_DIR) -I$(API_DIR) $(CFLAGS_EXTRA)
LDFLAGS += $(LDFLAGS_EXTRA)
CPPFLAGS += -I$(DATA_DIR)/src -I$(PLUGIN_DIR) -I$(API_DIR) $(CXXFLAGS_EXTRA)

ifneq ($(wildcard $(PLUGIN_DIR)/plugin_cxx.h),)
	CPPFLAGS += -DHAS_PLUGIN_CXX_H
endif

-include $(COMMON_DIR)/extra.mk
