#
# Tibia
#
# Copyright (C) 2023, 2024 Orastron Srl unipersonale
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

BUNDLE_NAME    := {{=it.product.bundleName}}

COMMON_DIR     := {{=it.vst3_make?.commonDir ?? (it.make?.commonDir ?? "")}}
DATA_DIR       := {{=it.vst3_make?.dataDir   ?? (it.make?.dataDir   ?? "")}}
PLUGIN_DIR     := {{=it.vst3_make?.pluginDir ?? (it.make?.pluginDir ?? "")}}

C_SRCS_EXTRA   := {{=it.make?.cSrcs    ?? ""}} {{=it.vst3_make?.cSrcs    ?? ""}}
M_SRCS_EXTRA   := {{=it.make?.mSrcs    ?? ""}} {{=it.vst3_make?.mSrcs    ?? ""}}
CXX_SRCS_EXTRA := {{=it.make?.cxxSrcs  ?? ""}} {{=it.vst3_make?.cxxSrcs  ?? ""}}

CFLAGS_EXTRA   := {{=it.make?.cflags   ?? ""}} {{=it.vst3_make?.cflags   ?? ""}}
CXXFLAGS_EXTRA := {{=it.make?.cxxflags ?? ""}} {{=it.vst3_make?.cxxflags ?? ""}}

LDFLAGS_EXTRA  := {{=it.make?.ldflags  ?? ""}} {{=it.vst3_make?.ldflags  ?? ""}}

HAS_UI         := {{=it.product.ui ? "yes" : "no"}}
