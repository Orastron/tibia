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

BUNDLE_NAME := {{=it.product.bundleName}}
JAVA_PACKAGE_NAME := {{=it.android.javaPackageName}}
ANDROID_VERSION := {{=it.android.androidVersion}}

{{?(it.android_make?.commonDir || it.make?.commonDir)}}
COMMON_DIR := {{=it.android_make?.commonDir ?? (it.make?.commonDir ?? "")}}
{{?}}
{{?(it.android_make?.dataDir || it.make?.dataDir)}}
DATA_DIR := {{=it.android_make?.dataDir ?? (it.make?.dataDir ?? "")}}
{{?}}
{{?(it.android_make?.pluginDir || it.make?.pluginDir)}}
PLUGIN_DIR := {{=it.android_make?.pluginDir ?? (it.make?.pluginDir ?? "")}}
{{?}}
{{?(it.android_make?.apiDir || it.make?.apiDir)}}
API_DIR := {{=it.android_make?.apiDir ?? (it.make?.apiDir ?? "")}}
{{?}}
{{?(it.android_make?.mkincDir || it.make?.mkincDir)}}
MKINC_DIR := {{=it.android_make?.mkincDir ?? (it.make?.mkincDir ?? "")}}
{{?}}

HAS_MIDI_IN := {{=it.product.buses.filter(x => x.type == "midi" && x.direction == "input").length > 0 ? "yes" : "no"}}

{{?it.make?.extra}}
{{=it.make.extra}}
{{?}}
{{?it.android_make?.extra}}
{{=it.android_make.extra}}
{{?}}
