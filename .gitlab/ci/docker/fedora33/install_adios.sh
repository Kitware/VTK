#!/bin/sh

readonly adios_repo="https://github.com/ornladios/ADIOS2"
readonly adios_commit="v2.7.1"

readonly adios_root="$HOME/adios"
readonly adios_src="$adios_root/src"
readonly adios_build_root="$adios_root/build"

git clone -b "$adios_commit" "$adios_repo" "$adios_src"

adios_build () {
    local subdir="$1"
    shift

    local prefix="$1"
    shift

    cmake -GNinja \
        -S "$adios_src" \
        -B "$adios_build_root/$subdir" \
        -DBUILD_SHARED_LIBS=ON \
        -DADIOS2_BUILD_EXAMPLES=OFF \
        -DBUILD_TESTING=OFF \
        -DCMAKE_BUILD_TYPE=Release \
        "-DCMAKE_INSTALL_PREFIX=$prefix" \
        -DADIOS2_USE_HDF5=ON \
        -DADIOS2_USE_ZeroMQ=OFF \
        -DADIOS2_USE_Python=OFF \
        -DADIOS2_USE_Fortran=OFF \
        -DADIOS2_USE_SST=OFF \
        -DADIOS2_USE_BZip2=OFF \
        -DADIOS2_USE_ZFP=OFF \
        -DADIOS2_USE_SZ=OFF \
        -DADIOS2_USE_MGARD=OFF \
        -DADIOS2_USE_PNG=OFF \
        -DADIOS2_USE_Blosc=OFF \
        -DADIOS2_USE_Endian_Reverse=OFF \
        -DADIOS2_USE_IME=OFF \
        "$@"
    cmake --build "$adios_build_root/$subdir"
    cmake --build "$adios_build_root/$subdir" --target install
}

# MPI-less
adios_build nompi /usr \
    -DADIOS2_USE_MPI=OFF

# MPICH
adios_build mpich /usr/lib64/mpich \
    -DADIOS2_USE_MPI=ON \
    -DCMAKE_INSTALL_LIBDIR=lib

# OpenMPI
adios_build openmpi /usr/lib64/openmpi \
    -DADIOS2_USE_MPI=ON \
    -DCMAKE_INSTALL_LIBDIR=lib

rm -rf "$adios_root"
