# Build
This example illustrates how to utilize multiple canvases in a single webassembly module. The key functions are:
1. `vtkWebAssemblyRenderWindowInteractor::SetCanvasSelector()`
2. `vtkWebAssemblyOpenGLRenderWindow::SetCanvasSelector()`

In WebGL, there can be atmost 16 contexts. As a result, the max.
number of canvases are limited. This example does not yet support webgpu, it will need slight
refactoring to make it compatible.

```
emcmake cmake \
  -G Ninja \
  -S /path/to/vtk/Examples/Emscripten/Cxx/MultipleCanvases \
  -B out/build \
  -DVTK_DIR=/path/to/where/vtk/wasm/was/built

cmake --build out/build
```

# Serve and test generated code

```
cd out/build
python3 -m http.server 8000
```

Open your browser to http://localhost:8000
