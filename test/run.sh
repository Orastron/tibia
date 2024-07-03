#!/bin/sh

dir=`dirname $0`
$dir/../tibia $dir/product.json,$dir/company.json,$dir/vst3.json $dir/../templates/vst3 $dir/../out/vst3
$dir/../tibia $dir/product.json,$dir/company.json,$dir/vst3.json,$dir/vst3-make.json $dir/../templates/vst3-make $dir/../out/vst3
cp $dir/plugin.h $dir/plugin_ui.h $dir/../out/vst3/src

$dir/../tibia $dir/product.json,$dir/company.json,$dir/lv2.json $dir/../templates/lv2 $dir/../out/lv2
$dir/../tibia $dir/product.json,$dir/company.json,$dir/lv2.json,$dir/lv2-make.json $dir/../templates/lv2-make $dir/../out/lv2
cp $dir/../templates/common/* $dir/../out/lv2/src/
cp $dir/plugin.h $dir/plugin_ui.h $dir/../out/lv2/src

$dir/../tibia $dir/product.json,$dir/company.json $dir/../templates/web $dir/../out/web
$dir/../tibia $dir/product.json,$dir/company.json $dir/../templates/web-make $dir/../out/web
$dir/../tibia $dir/product.json,$dir/company.json $dir/../templates/web-demo $dir/../out/web
cp $dir/plugin.h $dir/../out/web/src

$dir/../tibia $dir/product.json,$dir/company.json,$dir/android.json $dir/../templates/android $dir/../out/android
$dir/../tibia $dir/product.json,$dir/company.json,$dir/android.json,$dir/android-make.json $dir/../templates/android-make $dir/../out/android
cp $dir/keystore.jks $dir/../out/android
cp $dir/plugin.h $dir/../out/android/src

$dir/../tibia $dir/product.json,$dir/company.json,$dir/ios.json $dir/../templates/ios $dir/../out/ios
$dir/../tibia $dir/product.json,$dir/company.json,$dir/ios.json,$dir/ios-make.json $dir/../templates/ios-make $dir/../out/ios
cp $dir/plugin.h $dir/../out/ios/src

$dir/../tibia $dir/product.json,$dir/company.json $dir/../templates/cmd $dir/../out/cmd
$dir/../tibia $dir/product.json,$dir/company.json,$dir/cmd-make.json $dir/../templates/cmd-make $dir/../out/cmd
cp $dir/plugin.h $dir/../out/cmd/src

$dir/../tibia $dir/product.json,$dir/company.json,$dir/daisy-seed.json $dir/../templates/daisy-seed $dir/../out/daisy-seed
$dir/../tibia $dir/product.json,$dir/company.json,$dir/daisy-seed.json,$dir/daisy-seed-make.json $dir/../templates/daisy-seed-make $dir/../out/daisy-seed
cp $dir/plugin.h $dir/../out/daisy-seed/src
