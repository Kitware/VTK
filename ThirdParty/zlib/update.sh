#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="zlib"
readonly ownership="Zlib Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/zlib.git"
readonly tag="for/vtk"
readonly paths="
CMakeLists.txt

adler32.c
compress.c
crc32.c
deflate.c
gzclose.c
gzlib.c
gzread.c
gzwrite.c
infback.c
inffast.c
inflate.c
inftrees.c
trees.c
uncompr.c
zutil.c

crc32.h
deflate.h
gzguts.h
inffast.h
inffixed.h
inflate.h
inftrees.h
trees.h
zlib.h
zutil.h

zconf.h.cmakein

ChangeLog
README
FAQ
INDEX
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
