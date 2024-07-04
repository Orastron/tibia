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

BUNDLE_NAME                       := {{=it.product.bundleName}}
JAVA_PACKAGE_NAME                 := {{=it.android.javaPackageName}}

CFLAGS_EXTRA                      := {{=it.make?.cflags   ?? ""}} {{=it.android_make?.cflags   ?? ""}}
CXXFLAGS_EXTRA                    := {{=it.make?.cxxflags ?? ""}} {{=it.android_make?.cxxflags ?? ""}}
JFLAGS_EXTRA                      := {{=it.make?.jflags   ?? ""}} {{=it.android_make?.jflags   ?? ""}}
LDFLAGS_EXTRA                     := {{=it.make?.ldflags  ?? ""}} {{=it.android_make?.ldflags  ?? ""}}

C_SRCS_EXTRA                      := {{=it.make?.cSrcs   ?? ""}} {{=it.android_make?.cSrcs   ?? ""}}
CXX_SRCS_EXTRA                    := {{=it.make?.cxxSrcs ?? ""}} {{=it.android_make?.cxxSrcs ?? ""}}

COMMON_DIR                        := {{=it.android_make?.commonDir ?? (it.make?.commonDir ?? "")}}
DATA_DIR                          := {{=it.android_make?.dataDir   ?? (it.make?.dataDir   ?? "")}}
PLUGIN_DIR                        := {{=it.android_make?.pluginDir ?? (it.make?.pluginDir ?? "")}}

KEY_STORE                         := {{=it.android_make.keyStore}}
KEY_ALIAS                         := {{=it.android_make.keyAlias}}
STORE_PASS                        := {{=it.android_make.storePass}}
KEY_PASS                          := {{=it.android_make.keyPass}}

ANDROID_SDK_DIR                   := {{=it.android_make.sdkDir}}
ANDROID_NDK_DIR                   := ${ANDROID_SDK_DIR}/ndk/{{=it.android_make.ndkVersion}}
BUILD_TOOLS_DIR                   := ${ANDROID_SDK_DIR}/build-tools/{{=it.android_make.buildToolsVersion}}
ANDROIDX_DIR                      := {{=it.android_make.androidxDir}}
KOTLIN_DIR                        := {{=it.android_make.kotlinDir}}

ANDROID_JAR_FILE                  := ${ANDROID_SDK_DIR}/platforms/android-{{=it.android_make.androidVersion}}/android.jar
ANDROIDX_CORE_FILE                := ${ANDROIDX_DIR}/core-{{=it.android_make.androidxCoreVersion}}.jar
ANDROIDX_LIFECYCLE_COMMON_FILE    := ${ANDROIDX_DIR}/lifecycle-common-{{=it.android_make.androidxLifecycleCommonVersion}}.jar
ANDROIDX_VERSIONEDPARCELABLE_FILE := ${ANDROIDX_DIR}/versionedparcelable-{{=it.android_make.androidxVersionedparcelableVersion}}.jar
KOTLIN_STDLIB_FILE                := ${KOTLIN_DIR}/kotlin-stdlib-{{=it.android_make.kotlinStdlibVersion}}.jar
KOTLINX_COROUTINES_CORE_FILE      := ${KOTLIN_DIR}/kotlinx-coroutines-core-{{=it.android_make.kotlinxCoroutinesCoreVersion}}.jar
KOTLINX_COROUTINES_CORE_JVM_FILE  := ${KOTLIN_DIR}/kotlinx-coroutines-core-jvm-{{=it.android_make.kotlinxCoroutinesCoreJVMVersion}}.jar

HAS_MIDI_IN                       := {{=it.product.buses.filter(x => x.type == "midi" && x.direction == "input").length > 0 ? "yes" : "no"}}
