#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="jpeg"
readonly ownership="jpeg-turbo Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/libjpeg-turbo.git"
readonly tag="for/vtk-20260714-3.1.4.1"
readonly paths="
.gitattributes
CMakeLists.vtk.txt
src/vtk_jpeg_mangle.h

simd/arm/
simd/i386/
simd/nasm/
simd/x86_64/
simd/CMakeLists.txt
simd/jsimd.h

src/j*.c
src/j*.h
src/wrapper/j*-12.c
src/wrapper/j*-16.c
src/jconfig.h.in
src/jconfigint.h.in
src/jversion.h.in

LICENSE.md
README.ijg
README.md
README.kitware.md
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    rm -v src/*-tj.c
    mv -v CMakeLists.vtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
