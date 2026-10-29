#!/bin/bash

set -e
export CMAKE_CONFIGURATION="wasm${1}_emscripten_linux"

# ... bootstrap cmake/ninja/node/emsdk as needed (installs into /work/.gitlab)
[ -f .gitlab/cmake/bin/cmake ] || .gitlab/ci/cmake.sh
[ -f .gitlab/ninja ] || .gitlab/ci/ninja.sh
export PATH=$PWD/.gitlab:$PWD/.gitlab/cmake/bin:$PATH
cmake --version
ninja --version

[ -f .gitlab/node/bin/node ] || cmake -P .gitlab/ci/download_node.cmake
export NODE_DIR=$PWD/.gitlab/node
export PATH=$PWD/.gitlab/node/bin:$PATH
node --version

[ -f .gitlab/emsdk/emsdk ] || cmake -P .gitlab/ci/download_emsdk.cmake
$PWD/.gitlab/emsdk/emsdk install latest
export PATH=$PWD/.gitlab/emsdk/upstream/bin:$PWD/.gitlab/emsdk/upstream/emscripten:$PATH
clang --version
wasm-as --version
wasm-ld --version
wasm-opt --version
emcc --version
export EMCC_SKIP_SANITY_CHECK=1

[ -f .gitlab/sccache ] || .gitlab/ci/sccache.sh
sccache --start-server
sccache --show-stats
export PATH="/tmp/shim:$PATH"                        # sccache passthrough shim

export CMAKE_BUILD_TYPE=Release
BUILD=/output/build-${CMAKE_CONFIGURATION}-cacheprobe
emcmake cmake -G Ninja -S /work -B "$BUILD" \
  -C /work/.gitlab/ci/configure_${CMAKE_CONFIGURATION}.cmake \
  -DVTK_BUILD_TESTING=OFF

python3 /work/CMake/wasm/generate_emscripten_cache.py --build-dir "$BUILD"
