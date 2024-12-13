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

{{?(it.daisy_seed_make?.commonDir || it.make?.commonDir)}}
COMMON_DIR := {{=it.daisy_seed_make?.commonDir ?? (it.make?.commonDir ?? "")}}
{{?}}
{{?(it.daisy_seed_make?.dataDir || it.make?.dataDir)}}
DATA_DIR := {{=it.daisy_seed_make?.dataDir ?? (it.make?.dataDir ?? "")}}
{{?}}
{{?(it.daisy_seed_make?.pluginDir || it.make?.pluginDir)}}
PLUGIN_DIR := {{=it.daisy_seed_make?.pluginDir ?? (it.make?.pluginDir ?? "")}}
{{?}}
{{?(it.daisy_seed_make?.apiDir || it.make?.apiDir)}}
API_DIR := {{=it.daisy_seed_make?.apiDir ?? (it.make?.apiDir ?? "")}}
{{?}}
{{?(it.daisy_seed_make?.mkincDir || it.make?.mkincDir)}}
MKINC_DIR := {{=it.daisy_seed_make?.mkincDir ?? (it.make?.mkincDir ?? "")}}
{{?}}

{{?it.make?.extra}}
{{=it.make.extra}}
{{?}}
{{?it.daisy_seed_make?.extra}}
{{=it.daisy_seed_make.extra}}
{{?}}
