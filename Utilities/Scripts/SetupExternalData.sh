#!/usr/bin/env bash
#=============================================================================
# Copyright 2013 Kitware, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#=============================================================================

# Path conversion function.
case "$(uname)" in
  *CYGWIN*)
    native_path() {
      cygpath -m "$1"
    }
    ;;
  *MINGW*)
    native_path() {
      cmd //c echo "$1" | sed 's/^"//;s/"$//'
    }
    ;;
  *)
    native_path() {
      echo "$1"
    }
    ;;
esac

say_store() {
  if test -f "$config_store"; then
    echo 'A default for VTK_DATA_STORE is configured as:' &&
    echo &&
    sed 's/^/  /' < "$config_store" &&
    echo
  else
    echo 'No default for VTK_DATA_STORE is configured.' &&
    echo
  fi
}

ask_store() {
  ans='?'
  while test "$ans" = '?'; do
    read -ep 'From the options

 <empty>     = No change
 n,no        = No default
 h,home      = Use '"$store_home"'
 s,sibling   = Use '"$store_sibling"'
 <full-path> = Use specified <full-path>

select a default for VTK_DATA_STORE [n/h/s]: ' ans &&
    case "$ans" in
      n|N|no) ans='no' ;;
      h|H|home) ans="$store_home" ;;
      s|S|sibling) ans="$store_sibling" ;;
      /*) ;;
      [A-Za-z]:[/\\]*) ;;
      '') ans='' ;;
      *) echo; echo "Invalid response '$ans'"; echo; ans='?' ;;
    esac
  done
  eval "$1='$ans'"
}

cd "${BASH_SOURCE%/*}/../.." &&
config_store='.ExternalData/config/store' &&
store_home="$(native_path "${HOME-$USERPROFILE}/.ExternalData")" &&
store_sibling="$(native_path "${PWD%/*}/VTKExternalData")" &&
echo 'VTK may download data objects into a local "store" using
the CMake ExternalData module.  The store is content-addressed
and can be shared across multiple projects and build trees.
VTK build trees have a VTK_DATA_STORE CMake cache entry to set
their store location.  A default for this value to be used in
build trees created with this source tree may now be chosen.
' &&
say_store &&
ask_store ans &&
case "$ans" in
  '') ;;
  no) rm -f "$config_store" ;;
  *)  mkdir -p "${config_store%/*}" && echo "$ans" > "$config_store" ;;
esac &&
say_store

say_exclude() {
  if test -f "$config_exclude"; then
    echo 'A default for VTK_DATA_EXCLUDE_FROM_ALL is configured as:' &&
    echo &&
    sed 's/^/  /' < "$config_exclude" &&
    echo
  else
    echo 'No default for VTK_DATA_EXCLUDE_FROM_ALL is configured.' &&
    echo
  fi
}

ask_exclude() {
  ans='?'
  while test "$ans" = '?'; do
    read -ep 'From the options

 <empty>     = No change
 d,delete    = No default or delete current default
 n,no,off    = Include VTKData target in default build
 y,yes,on    = Exclude VTKData target from default build

select a default for VTK_DATA_EXCLUDE_FROM_ALL [d/n/y]: ' ans &&
    case "$ans" in
      d|D|delete) ans='D' ;;
      n|N|no|off) ans='OFF' ;;
      y|Y|yes|on) ans='ON' ;;
      '') ans='' ;;
      *) echo; echo "Invalid response '$ans'"; echo; ans='?' ;;
    esac
  done
  eval "$1='$ans'"
}


cd "${BASH_SOURCE%/*}/../.." &&
config_exclude='.ExternalData/config/exclude-from-all' &&
echo 'VTK defines a "VTKData" build target to download data
objects at build time to make them available for running tests.
VTK build trees have a VTK_DATA_EXCLUDE_FROM_ALL CMake
cache option to exclude the "VTKData" target from being built
as part of the default ("all") build.  A default for this value to
be used in build trees created with this source tree may now be
chosen.
' &&
say_exclude &&
ask_exclude ans &&
case "$ans" in
  '') ;;
  D) rm -f "$config_exclude" ;;
  *)  mkdir -p "${config_exclude%/*}" && echo "$ans" > "$config_exclude" ;;
esac &&
say_exclude
