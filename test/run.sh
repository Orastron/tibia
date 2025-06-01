#!/bin/sh

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

dir=`dirname $0`

$dir/../tibia $dir/product.json,$dir/company.json $dir/../templates/api $dir/../out/api

$dir/../tibia $dir/product.json,$dir/company.json,$dir/vst3.json $dir/../templates/vst3 $dir/../out/vst3
$dir/../tibia $dir/product.json,$dir/company.json,$dir/vst3.json $dir/../templates/vst3-make $dir/../out/vst3
cp $dir/plugin.h $dir/plugin_ui.h $dir/../out/vst3/src
cp $dir/vars-pre.mk $dir/../out/vst3

$dir/../tibia $dir/product.json,$dir/company.json,$dir/lv2.json $dir/../templates/lv2 $dir/../out/lv2
$dir/../tibia $dir/product.json,$dir/company.json,$dir/lv2.json $dir/../templates/lv2-make $dir/../out/lv2
cp $dir/plugin.h $dir/plugin_ui.h $dir/../out/lv2/src
cp $dir/vars-pre.mk $dir/../out/lv2

$dir/../tibia $dir/product.json,$dir/company.json $dir/../templates/web $dir/../out/web
$dir/../tibia $dir/product.json,$dir/company.json $dir/../templates/web-make $dir/../out/web
$dir/../tibia $dir/product.json,$dir/company.json $dir/../templates/web-demo $dir/../out/web
cp $dir/plugin.h $dir/../out/web/src
cp $dir/vars-pre.mk $dir/vars-extra.mk $dir/rules-extra.mk $dir/../out/web

$dir/../tibia $dir/product.json,$dir/company.json,$dir/android.json $dir/../templates/android $dir/../out/android
$dir/../tibia $dir/product.json,$dir/company.json,$dir/android.json $dir/../templates/android-make $dir/../out/android
cp $dir/keystore.jks $dir/../out/android
cp $dir/plugin.h $dir/../out/android/src
cp $dir/vars-pre.mk $dir/../out/android

$dir/../tibia $dir/product.json,$dir/company.json,$dir/ios.json $dir/../templates/ios $dir/../out/ios
$dir/../tibia $dir/product.json,$dir/company.json,$dir/ios.json,$dir/ios-make.json $dir/../templates/ios-make $dir/../out/ios
cp $dir/plugin.h $dir/../out/ios/src
cp $dir/vars-pre.mk $dir/../out/ios

$dir/../tibia $dir/product.json,$dir/company.json $dir/../templates/cmd $dir/../out/cmd
$dir/../tibia $dir/product.json,$dir/company.json $dir/../templates/cmd-make $dir/../out/cmd
cp $dir/plugin.h $dir/../out/cmd/src
cp $dir/vars-pre.mk $dir/../out/cmd

$dir/../tibia $dir/product.json,$dir/company.json,$dir/daisy-seed.json $dir/../templates/daisy-seed $dir/../out/daisy-seed
$dir/../tibia $dir/product.json,$dir/company.json,$dir/daisy-seed.json $dir/../templates/daisy-seed-make $dir/../out/daisy-seed
cp $dir/plugin.h $dir/../out/daisy-seed/src
cp $dir/vars-pre.mk $dir/../out/daisy-seed
