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

BUNDLE_NAME := {{=it.product.bundleName}}

{{?(it.ios_make?.commonDir || it.make?.commonDir)}}
COMMON_DIR := {{=it.ios_make?.commonDir ?? (it.make?.commonDir ?? "")}}
{{?}}
{{?(it.ios_make?.dataDir || it.make?.dataDir)}}
DATA_DIR := {{=it.ios_make?.dataDir ?? (it.make?.dataDir ?? "")}}
{{?}}
{{?(it.ios_make?.pluginDir || it.make?.pluginDir)}}
PLUGIN_DIR := {{=it.ios_make?.pluginDir ?? (it.make?.pluginDir ?? "")}}
{{?}}
{{?(it.ios_make?.mkincDir || it.make?.mkincDir)}}
MKINC_DIR := {{=it.ios_make?.mkincDir ?? (it.make?.mkincDir ?? "")}}
{{?}}

{{?(it.ios_make?.extra || it.make?.extra)}}
{{=it.ios_make?.extra ?? (it.make?.extra ?? "")}}
{{?}}
