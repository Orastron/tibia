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

BUNDLE_NAME     := {{=it.product.bundleName}}

CFLAGS_EXTRA    := {{=it.make?.cflags   ?? ""}} {{=it.cmd_make?.cflags   ?? ""}}
CXXFLAGS_EXTRA  := {{=it.make?.cxxflags ?? ""}} {{=it.cmd_make?.cxxflags ?? ""}}
LDFLAGS_EXTRA   := {{=it.make?.ldflags  ?? ""}} {{=it.cmd_make?.ldflags  ?? ""}}

C_SRCS_EXTRA    := {{=it.make?.cSrcs    ?? ""}} {{=it.cmd_make?.cSrcs   ?? ""}}
CXX_SRCS_EXTRA  := {{=it.make?.cxxSrcs  ?? ""}} {{=it.cmd_make?.cxxSrcs ?? ""}}

COMMON_DIR      := {{=it.cmd_make?.commonDir ?? (it.make?.commonDir ?? "")}}
DATA_DIR        := {{=it.cmd_make?.dataDir   ?? (it.make?.dataDir   ?? "")}}
PLUGIN_DIR      := {{=it.cmd_make?.pluginDir ?? (it.make?.pluginDir ?? "")}}

TINYWAV_DIR     := {{=it.cmd_make.tinywavDir}}
MIDI_PARSER_DIR := {{=it.cmd_make.midiParserDir}}

HAS_MIDI_IN     := {{=it.product.buses.filter(x => x.type == "midi" && x.direction == "input").length > 0 ? "yes" : "no"}}
