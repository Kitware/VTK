#!/bin/sh

set -e
set -x

readonly vtkroot="/vtk"
readonly vtksrc="$vtkroot/src"
readonly vtkout="$vtkroot/out"

# Install development tools.
yum install -y gcc-c++ ninja-build

readonly cmake_ver="3.17.3"

# Download and install cmake
cd "$vtkroot"
curl -OL "https://github.com/Kitware/CMake/releases/download/v$cmake_ver/cmake-$cmake_ver-Linux-x86_64.tar.gz"
mkdir -p "/opt/cmake"
tar --strip-components=1 -C /opt/cmake -xzf "cmake-$cmake_ver-Linux-x86_64.tar.gz"

for pyver in 36 37 38; do
    suffix=""
    [ "$pyver" -le 37 ] && suffix="m"
    pyroot="/opt/python/cp$pyver-cp$pyver$suffix"

    builddir="$vtkroot/build-$pyver"
    mkdir -p "$builddir"
    cd "$builddir"
    /opt/cmake/bin/cmake -GNinja \
        -DCMAKE_BUILD_TYPE:STRING=Release \
        -DVTK_WHEEL_BUILD:BOOL=ON \
        -DVTK_ENABLE_WRAPPING:BOOL=ON \
        -DVTK_WRAP_PYTHON:BOOL=ON \
        -DVTK_PYTHON_VERSION:STRING=3 \
        "-DCMAKE_PREFIX_PATH:PATH=$pyroot" \
        -DPython3_FIND_STRATEGY:STRING=LOCATION \
        "$vtksrc"
    ninja-build
    "$pyroot/bin/python" setup.py bdist_wheel
    auditwheel addtag dist/*.whl
    mv -v wheelhouse/*.whl dist/*.whl "$vtkout"
done
