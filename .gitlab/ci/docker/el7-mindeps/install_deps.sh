#!/bin/sh

set -e

# Install extra dependencies for VTK
yum install -y --setopt=install_weak_deps=False \
    bzip2 patch git-core

# Core build tools
yum install -y --setopt=install_weak_deps=False \
    gcc gcc-c++

# MPI dependencies
yum install -y --setopt=install_weak_deps=False \
    openmpi-devel mpich-devel

# Qt dependencies
yum install -y --setopt=install_weak_deps=False \
    qt5-qtbase-devel qt5-qttools-devel qt5-qtquickcontrols2-devel

# Mesa dependencies
yum install -y --setopt=install_weak_deps=False \
    mesa-libOSMesa-devel mesa-libOSMesa mesa-dri-drivers mesa-libGL* glx-utils

# External dependencies
yum install -y --setopt=install_weak_deps=False \
    libXcursor-devel libtiff-devel lz4-devel expat-devel glew-devel \
    libogg-devel libtheora-devel protobuf-devel libxkbcommon boost-devel \
    tbb-devel libiodbc-devel libarchive-devel freeglut-devel sqlite-devel \
    fontconfig-devel

# Python dependencies
yum install -y --setopt=install_weak_deps=False \
    python3 python3-devel python3-numpy \
    python3-pip python3-mpi4py-mpich python3-mpi4py-openmpi python3-matplotlib

# EPEL for more tools
yum install -y --setopt=install_weak_deps=False \
    epel-release

# Development tools not in EL7
yum install -y --setopt=install_weak_deps=False \
    git-lfs

# External dependencies not in EL7
yum install -y --setopt=install_weak_deps=False \
    openslide-devel liblas-devel liblas-devel libgeotiff-devel gdal-devel \
    laszip-devel

yum clean all
