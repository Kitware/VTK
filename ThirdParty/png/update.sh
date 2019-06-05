#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="png"
readonly ownership="Libpng Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/png.git"
readonly tag="for/vtk-20190605-1.6.37"
readonly paths="
.gitattributes
CMakeLists.vtk.txt

scripts/dfn.awk
scripts/options.awk
scripts/pnglibconf.dfa
scripts/pnglibconf.mak
pngusr.dfa

CHANGES
LICENSE
README
README.kitware.md

png.c
pngerror.c
pngget.c
pngmem.c
pngpread.c
pngread.c
pngrio.c
pngrtran.c
pngrutil.c
pngset.c
pngtrans.c
pngwio.c
pngwrite.c
pngwtran.c
pngwutil.c

png.h
pngconf.h
pngdebug.h
pnginfo.h
pngpriv.h
pngstruct.h
vtk_png_mangle.h
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mkdir -p "vtkzlib"
    sed -e 's/#cmakedefine/#undef/' \
        "${BASH_SOURCE%/*}/../zlib/vtkzlib/zconf.h.cmakein" \
        > vtkzlib/zconf.h
    CPPFLAGS="-I${BASH_SOURCE%/*}/../zlib/vtkzlib -I$PWD -DZ_PREFIX -DZ_HAVE_UNISTD_H" make -f scripts/pnglibconf.mak
    rm -rvf "vtkzlib"
    sed -i -e '/PNG_ZLIB_VERNUM/s/0x.*/0/' pnglibconf.h
    rm -rvf scripts pngusr.dfa pnglibconf.dfn pnglibconf.pre pnglibconf.out
    mv -v CMakeLists.vtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
