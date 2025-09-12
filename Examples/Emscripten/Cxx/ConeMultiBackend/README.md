# WebAssembly Cone Example

This guide illustrates the process of developing a 3D object viewer tailored for WebAssembly. Notably, it offers the flexibility to select the rendering backend, either OpenGL or WebGPU, during runtime.

The additional steps required are:

1. Update `CMakeLists.txt` so that the `VTK::RenderingWebGPU` component is requested when finding VTK.
2. Link your targets with `RenderingWebGPU`. There is no problem linking to both `RenderingWebGPU` and `RenderingOpenGL2`.
3. Preload environment variable with `VTK_GRAPHICS_BACKEND` set to 'WEBGPU'.

## Build

### Using local VTK WASM build tree

```
emcmake cmake \
  -G Ninja \
  -S /path/to/vtk/Examples/Emscripten/Cxx/ConeMultiBackend \
  -B out/build \
  -DVTK_DIR=/path/to/where/vtk/wasm/was/built

cmake --build out/build
```

### Using vtk-wasm-sdk

#### Linux/macOS
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

#### Windows
```sh
docker run --rm -it `
  -v"$PWD":/work kitware/vtk-wasm-sdk:latest `
  emcmake cmake -GNinja -S /work -B /work/build -DVTK_DIR=/VTK-install/Release/wasm32/lib/cmake/vtk
# Build
docker run --rm -it `
  -v"$PWD":/work kitware/vtk-wasm-sdk:latest `
  cmake --build /work/build
```

## Serve and test generated code

```
cd out/build
python3 -m http.server 8000
```

Open your browser to http://localhost:8000
