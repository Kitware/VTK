#!/bin/sh

set -e

# mirrorlist.centos.org no longer exists because el7 is past end of life.
# To get packages, replace mirrorlist with baseurl and change mirror.centos.org
# to vault.centos.org.
sed -i s/mirror.centos.org/vault.centos.org/g /etc/yum.repos.d/CentOS-*.repo
sed -i s/^#.*baseurl=http/baseurl=http/g /etc/yum.repos.d/CentOS-*.repo
sed -i s/^mirrorlist=http/#mirrorlist=http/g /etc/yum.repos.d/CentOS-*.repo

# Install extra dependencies for VTK
yum install -y --setopt=install_weak_deps=False \
    bzip2 patch git-core

# To install devtools-8 and Python 3.8 via scl
yum install -y --setopt=install_weak_deps=False \
    centos-release-scl

# Clean up additional repos configured by centos-release-scl
sed -i s/mirror.centos.org/vault.centos.org/g /etc/yum.repos.d/CentOS-*.repo
sed -i s/^#.*baseurl=http/baseurl=http/g /etc/yum.repos.d/CentOS-*.repo
sed -i s/^mirrorlist=http/#mirrorlist=http/g /etc/yum.repos.d/CentOS-*.repo

# Core build tools
yum install -y --setopt=install_weak_deps=False \
    devtoolset-8

# Modern git version
yum install -y --setopt=install_weak_deps=False \
    rh-git227

yum install -y --setopt=install_weak_deps=False \
    rh-python38-python rh-python38-python-devel \
    rh-python38-python-pip rh-python38-python-numpy

# Install a venv which provides `matplotlib`
scl enable rh-python38 -- python3 -m venv /opt/python38/venv
/opt/python38/venv/bin/pip install matplotlib
# Ignore mpi4py for now as the CI job doesn't build with MPI anyways.
# scl enable rh-python38 -- /opt/python38/venv/bin/pip install mpi4py

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
