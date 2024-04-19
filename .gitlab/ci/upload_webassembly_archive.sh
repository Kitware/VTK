#!/bin/sh

set -e
set -x

version=$( node --eval "import('./build/bin/vtkWasmSceneManager.mjs').then(m => m.default().then(i => console.log(i.getVTKVersion())))" )
readonly version
readonly package_name="vtk-wasm"

cd build/bin
curl --header "JOB-TOKEN: $CI_JOB_TOKEN" --upload-file "$package_name.$version.tar.gz" "$CI_API_V4_URL/projects/$CI_PROJECT_ID/packages/generic/$package_name/$version/$package_name.$version.tar.gz"
cd ../../
