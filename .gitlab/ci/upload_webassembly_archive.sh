#!/bin/sh

set -e
set -x

# Select the wasm architecture for the package.
readonly architecture="wasm${1:-32}-emscripten"
case "$architecture" in
    wasm64-emscripten)
        node_args="--experimental-wasm-memory64"
        ;;
    *)
        node_args=""
        ;;
esac
readonly node_args
version=$( node $node_args --eval "import('./build/install/bin/vtkWasmSceneManager.mjs').then(m => m.default().then(i => console.log(i.getVTKVersion())))" )
readonly version
readonly prefix="vtk"
readonly package_name="$prefix-$architecture"

cd build/install/bin
curl --header "JOB-TOKEN: $CI_JOB_TOKEN" --upload-file "$prefix-$version-$architecture.tar.gz" "$CI_API_V4_URL/projects/$CI_PROJECT_ID/packages/generic/$package_name/$version/$prefix-$version-$architecture.tar.gz"
cd ../../
