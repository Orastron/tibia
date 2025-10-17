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

$dir/../tibia $dir/product.json,$dir/company.json,$dir/vst3.json $dir/../templates/vst3 $dir/../out/vst3/c
$dir/../tibia $dir/product.json,$dir/company.json,$dir/vst3.json $dir/../templates/vst3-make $dir/../out/vst3/c
cp $dir/plugin.h $dir/plugin_ui.h $dir/../out/vst3/c/src
cp $dir/vars-pre.mk $dir/../out/vst3/c

$dir/../tibia $dir/product.json,$dir/company.json,$dir/vst3.json $dir/../templates/vst3 $dir/../out/vst3/cxx
$dir/../tibia $dir/product.json,$dir/company.json,$dir/vst3.json $dir/../templates/vst3-make $dir/../out/vst3/cxx
cp $dir/plugin_cxx.h $dir/../out/vst3/cxx/src
cp $dir/plugin_ui.h $dir/../out/vst3/cxx/src/plugin_ui_cxx.h
cp $dir/vars-pre.mk $dir/../out/vst3/cxx

$dir/../tibia $dir/product.json,$dir/company.json,$dir/lv2.json $dir/../templates/lv2 $dir/../out/lv2/c
$dir/../tibia $dir/product.json,$dir/company.json,$dir/lv2.json $dir/../templates/lv2-make $dir/../out/lv2/c
cp $dir/plugin.h $dir/plugin_ui.h $dir/../out/lv2/c/src
cp $dir/vars-pre.mk $dir/../out/lv2/c

$dir/../tibia $dir/product.json,$dir/company.json,$dir/lv2.json $dir/../templates/lv2 $dir/../out/lv2/cxx
$dir/../tibia $dir/product.json,$dir/company.json,$dir/lv2.json $dir/../templates/lv2-make $dir/../out/lv2/cxx
cp $dir/plugin_cxx.h $dir/../out/lv2/cxx/src
cp $dir/plugin_ui.h $dir/../out/lv2/cxx/src/plugin_ui_cxx.h
cp $dir/vars-pre.mk $dir/../out/lv2/cxx

$dir/../tibia $dir/product.json,$dir/company.json $dir/../templates/web $dir/../out/web/c
$dir/../tibia $dir/product.json,$dir/company.json $dir/../templates/web-make $dir/../out/web/c
$dir/../tibia $dir/product.json,$dir/company.json $dir/../templates/web-demo $dir/../out/web/c
cp $dir/plugin.h $dir/../out/web/c/src
cp $dir/vars-pre.mk $dir/vars-extra.mk $dir/rules-extra.mk $dir/../out/web/c

$dir/../tibia $dir/product.json,$dir/company.json $dir/../templates/web $dir/../out/web/cxx
$dir/../tibia $dir/product.json,$dir/company.json $dir/../templates/web-make $dir/../out/web/cxx
$dir/../tibia $dir/product.json,$dir/company.json $dir/../templates/web-demo $dir/../out/web/cxx
cp $dir/plugin_cxx.h $dir/../out/web/cxx/src
cp $dir/vars-pre.mk $dir/vars-extra.mk $dir/rules-extra.mk $dir/../out/web/cxx

$dir/../tibia $dir/product.json,$dir/company.json,$dir/android.json $dir/../templates/android $dir/../out/android/c
$dir/../tibia $dir/product.json,$dir/company.json,$dir/android.json $dir/../templates/android-make $dir/../out/android/c
cp $dir/keystore.jks $dir/../out/android/c
cp $dir/plugin.h $dir/../out/android/c/src
cp $dir/vars-pre.mk $dir/../out/android/c

$dir/../tibia $dir/product.json,$dir/company.json,$dir/android.json $dir/../templates/android $dir/../out/android/cxx
$dir/../tibia $dir/product.json,$dir/company.json,$dir/android.json $dir/../templates/android-make $dir/../out/android/cxx
cp $dir/keystore.jks $dir/../out/android/cxx
cp $dir/plugin_cxx.h $dir/../out/android/cxx/src
cp $dir/vars-pre.mk $dir/../out/android/cxx

$dir/../tibia $dir/product.json,$dir/company.json,$dir/ios.json $dir/../templates/ios $dir/../out/ios/c
$dir/../tibia $dir/product.json,$dir/company.json,$dir/ios.json,$dir/ios-make.json $dir/../templates/ios-make $dir/../out/ios/c
cp $dir/plugin.h $dir/../out/ios/c/src
cp $dir/vars-pre.mk $dir/../out/ios/c

$dir/../tibia $dir/product.json,$dir/company.json,$dir/ios.json $dir/../templates/ios $dir/../out/ios/cxx
$dir/../tibia $dir/product.json,$dir/company.json,$dir/ios.json,$dir/ios-make.json $dir/../templates/ios-make $dir/../out/ios/cxx
cp $dir/plugin_cxx.h $dir/../out/ios/cxx/src
cp $dir/vars-pre.mk $dir/../out/ios/cxx

$dir/../tibia $dir/product.json,$dir/company.json $dir/../templates/cmd $dir/../out/cmd/c
$dir/../tibia $dir/product.json,$dir/company.json $dir/../templates/cmd-make $dir/../out/cmd/c
cp $dir/plugin.h $dir/../out/cmd/c/src
cp $dir/vars-pre.mk $dir/../out/cmd/c

$dir/../tibia $dir/product.json,$dir/company.json $dir/../templates/cmd $dir/../out/cmd/cxx
$dir/../tibia $dir/product.json,$dir/company.json $dir/../templates/cmd-make $dir/../out/cmd/cxx
cp $dir/plugin_cxx.h $dir/../out/cmd/cxx/src
cp $dir/vars-pre.mk $dir/../out/cmd/cxx

$dir/../tibia $dir/product.json,$dir/company.json,$dir/daisy-seed.json $dir/../templates/daisy-seed $dir/../out/daisy-seed/c
$dir/../tibia $dir/product.json,$dir/company.json,$dir/daisy-seed.json $dir/../templates/daisy-seed-make $dir/../out/daisy-seed/c
cp $dir/plugin.h $dir/../out/daisy-seed/c/src
cp $dir/vars-pre.mk $dir/../out/daisy-seed/c

$dir/../tibia $dir/product.json,$dir/company.json,$dir/daisy-seed.json $dir/../templates/daisy-seed $dir/../out/daisy-seed/cxx
$dir/../tibia $dir/product.json,$dir/company.json,$dir/daisy-seed.json $dir/../templates/daisy-seed-make $dir/../out/daisy-seed/cxx
cp $dir/plugin_cxx.h $dir/../out/daisy-seed/cxx/src
cp $dir/vars-pre.mk $dir/../out/daisy-seed/cxx
