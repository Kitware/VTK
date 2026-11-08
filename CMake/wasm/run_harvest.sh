#!/bin/bash

set -e
set -x
WORDSIZE=$1
SCRATCH=$PWD/vtk-wasm-$WORDSIZE-cacheprobe
mkdir -p "$SCRATCH"
docker run --rm \
  -v ~/dev/v/vtk:/work:Z \
  -v "$SCRATCH":/output:Z \
  -w /work \
  -u "$(id -u):$(id -g)" \
  kitware/vtk:ci-fedora44-20260903 \
  /work/CMake/wasm/harvest_emsdk_cache.sh $WORDSIZE
