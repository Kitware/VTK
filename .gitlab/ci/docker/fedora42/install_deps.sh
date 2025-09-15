#!/bin/sh

set -e

# Install extra dependencies for VTK
dnf install -y --setopt=install_weak_deps=False \
    bzip2 patch git-core git-lfs

# Documentation tools
dnf install -y --setopt=install_weak_deps=False \
    doxygen perl-Digest-MD5

# Development tools
dnf install -y --setopt=install_weak_deps=False \
    libasan libtsan libubsan clang-tools-extra

# MPI dependencies
dnf install -y --setopt=install_weak_deps=False \
    openmpi-devel mpich-devel

# Mesa dependencies
dnf install -y --setopt=install_weak_deps=False \
    mesa-libOSMesa-devel mesa-libOSMesa mesa-dri-drivers mesa-libGL* glx-utils

# External dependencies
dnf install -y --setopt=install_weak_deps=False \
    libXcursor-devel libharu-devel utf8cpp-devel pugixml-devel libtiff-devel \
    eigen3-devel lz4-devel expat-devel glew-devel \
    hdf5-devel hdf5-mpich-devel hdf5-openmpi-devel hdf5-devel netcdf-devel \
    netcdf-mpich-devel netcdf-openmpi-devel libogg-devel libtheora-devel \
    jsoncpp-devel gl2ps-devel protobuf-devel libxkbcommon libxcrypt-compat \
    boost-devel postgresql-server-devel postgresql-private-devel \
    mariadb-devel libiodbc-devel PDAL-devel liblas-devel openslide-devel \
    libarchive-devel freeglut-devel sqlite-devel PEGTL-devel cgnslib-devel \
    proj-devel wkhtmltopdf cli11-devel fmt-devel openvdb-devel json-devel \
    openxr openxr-devel libscn-devel

# Python dependencies
dnf install -y --setopt=install_weak_deps=False \
    python3 python3-devel python3-numpy python3-tkinter \
    python3-pip python3-mpi4py-mpich python3-mpi4py-openmpi python3-matplotlib

# CI dependencies packages
dnf install -y --setopt=install_weak_deps=False \
    ninja-build cmake

# Tcl/Tk dependencies (for building RenderingTk)
dnf install -y --setopt=install_weak_deps=False \
    tcl-devel tk-devel

# wslink will bring aiohttp>=3.7.4
python3 -m pip install 'wslink>=1.0.4'

# C++ dependencies (along GCC 15 which is the default)
dnf install -y --setopt=install_weak_deps=False \
    gcc14 gcc14-c++

# Java dependencies
dnf install --releasever=41 -y --setopt=install_weak_deps=False \
    java-11-openjdk-devel xmlstarlet

# RPMFusion (for ffmpeg)
dnf install -y --setopt=install_weak_deps=False \
    https://mirrors.rpmfusion.org/free/fedora/rpmfusion-free-release-42.noarch.rpm

# RPMFusion external dependencies
dnf install -y --setopt=install_weak_deps=False \
    ffmpeg-devel

# External repository support
dnf install -y --setopt=install_weak_deps=False \
    dnf-plugins-core

if [ "$( uname -m )" = "x86_64" ]; then
    # Openturns dependencies
    dnf config-manager addrepo \
        --from-repofile=https://download.opensuse.org/repositories/science:/openturns/Fedora_42/science:openturns.repo
    dnf install -y --setopt=install_weak_deps=False \
        openturns-libs openturns-devel
fi

# Vulkan backend dependencies
dnf install -y --setopt=install_weak_deps=False \
    mesa-vulkan-drivers

# Emscripten SDK dependencies
dnf install -y --setopt=install_weak_deps=False \
    xz

dnf clean all
