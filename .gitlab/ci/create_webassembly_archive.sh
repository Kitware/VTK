#!/bin/sh

set -e
set -x

version=$( node --eval "import('./build/bin/vtkWasmSceneManager.mjs').then(m => m.default().then(i => console.log(i.getVTKVersion())))" )
readonly version

cd build/bin
tar -cvzf "vtk-wasm.$version.tar.gz" ./vtkWasmSceneManager.*
cd ../../
