# Building WebAssembly examples with rendering

These examples are self-contained and serve the purpose of demonstrating how to utilize VTK for rendering tasks, all within a browser environment through the utilization of WebAssembly.

These examples were contributed by Rostyslav Lyulinetskyy and Ilya Volkov from
https://about.dicehub.com/.

## Compiling VTK for WebAssembly architecture

Please refer to [Documentation/dev/build_wasm_emscripten.md](https://gitlab.kitware.com/vtk/-/blob/master/Documentation/dev/build_wasm_emscripten.md) for
detailed instructions.

## Examples
- [AsyncClipper](./AsyncClipper/README.md) App that uses a clip widget to clip a mesh asynchronously without blocking main UI thread.
- [Cone](./Cone/README.md) Shows how to write an extremely simple 3D object viewer that runs in the browser using VTK that targets WebAssembly architecture.
- [ConeMultiBackend](./ConeMultiBackend/README.md) Shows how to write a 3D object viewer for
WebAssembly . Notably, it offers the flexibility to select the rendering backend, either OpenGL or WebGPU, during runtime.
- [GeometryViewer](./GeometryViewer/README.md) Load files from popular formats and adjust rendering options.
- [ModuleTesting](./ModuleTesting/README.md) Illustrates necessary CMake code and directory structure to enable and run WASM tests for custom modules.
- [MultiCone](./MultiCone/README.md) Shows to render multiple viewports in a HTML canvas using a VTK wasm module per viewport.
- [MultipleCanvases](./MultipleCanvases/README.md) Shows how to use multiple canvases in a single WASM module.
- [WrappedAsyncClipper](./WrappedAsyncClipper/README.md) Same as AsyncClipper but without a `main()` function and instead uses wrapped methods.
- [WrappedMace](./WrappedMace/README.md) Shows how to write a 3D object viewer in JavaScript using wrapped VTK C++ classes.
