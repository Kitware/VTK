#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="freetype"
readonly ownership="Freetype Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/freetype.git"
readonly tag="for/vtk-20210729-2.11.0"
readonly paths="
include
src

docs/FTL.TXT
LICENSE.TXT

builds/unix/ftsystem.c
builds/unix/ftconfig.h.in
builds/windows/ftdebug.c

CMakeLists.vtk.txt
README
README.kitware.md
.gitattributes
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    # Remove remnants of upstream's build.
    find -name rules.mk -delete
    find -name module.mk -delete
    find -name Jamfile -delete
    # Remove unused headers.
    rm -rvf include/freetype/ftchapters.h
    # Remove unused modules.
    rm -rvf src/bzip2
    rm -rvf src/tools
    # Remove freetype's import zlib.
    rm -rvf src/gzip/adler32.c
    rm -rvf src/gzip/ftzconf.h
    rm -rvf src/gzip/inf*
    rm -rvf src/gzip/z*
    mv -v CMakeLists.vtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
