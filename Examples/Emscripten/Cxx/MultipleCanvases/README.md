# Build
This example illustrates how to utilize multiple canvases in a single webassembly module. The key functions are:
1. `vtkWebAssemblyRenderWindowInteractor::SetCanvasSelector()`
2. `vtkWebAssemblyOpenGLRenderWindow::SetCanvasSelector()`

In WebGL, there can be atmost 16 contexts. As a result, the max.
number of canvases are limited. This example does not yet support webgpu, it will need slight
refactoring to make it compatible.

## Using local VTK WASM build tree

```
emcmake cmake \
  -G Ninja \
  -S /path/to/vtk/Examples/Emscripten/Cxx/MultipleCanvases \
  -B out/build \
  -DVTK_DIR=/path/to/where/vtk/wasm/was/built

cmake --build out/build
```

## Using vtk-wasm-sdk

### Linux/macOS
```sh
# Configure
docker run --rm -it \
  -v"$PWD":/work kitware/vtk-wasm-sdk:latest \
  emcmake cmake -GNinja -S /work -B /work/build -DVTK_DIR=/VTK-install/Release/wasm32/lib/cmake/vtk
# Build
docker run --rm -it \
  -v"$PWD":/work kitware/vtk-wasm-sdk:latest \
  cmake --build /work/build
```

### Windows
```sh
docker run --rm -it `
  -v"$PWD":/work kitware/vtk-wasm-sdk:latest `
  emcmake cmake -GNinja -S /work -B /work/build -DVTK_DIR=/VTK-install/Release/wasm32/lib/cmake/vtk
# Build
docker run --rm -it `
  -v"$PWD":/work kitware/vtk-wasm-sdk:latest `
  cmake --build /work/build
```

# Serve and test generated code

```
cd out/build
python3 -m http.server 8000
```

Open your browser to http://localhost:8000
