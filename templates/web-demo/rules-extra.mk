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

build/web/index.html: $(DATA_DIR)/src/index.html | build/web
	cp $^ $@

build/web/key.pem: build/web/cert.pem

build/web/cert.pem: | build
	yes "" | openssl req -x509 -newkey rsa:2048 -keyout build/web/key.pem -out build/web/cert.pem -days 365 -nodes 2>/dev/null
