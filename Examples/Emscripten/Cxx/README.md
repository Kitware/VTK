# Building WebAssembly examples with rendering

These examples are self-contained and serve the purpose of demonstrating how to utilize VTK for rendering tasks, all within a browser environment through the utilization of WebAssembly.

These examples were contributed by Rostyslav Lyulinetskyy and Ilya Volkov from
https://about.dicehub.com/.

## Compiling VTK for WebAssembly architecture

Please refer to [Documentation/dev/build_wasm_emscripten.md](https://gitlab.kitware.com/vtk/-/blob/master/Documentation/dev/build_wasm_emscripten.md) for
detailed instructions.

## Examples
- [Cone](./Cone/README.md) Shows how to write an extremely simple 3D object viewer that runs in the browser using VTK that targets WebAssembly architecture.
- [ConeMultiBackend](./ConeMultiBackend/README.md) Shows how to write a 3D object viewer for
WebAssembly . Notably, it offers the flexibility to select the rendering backend, either OpenGL or WebGPU, during runtime.
- [MultiCone](./MultiCone/README.md) Shows to render multiple viewports in a HTML canvas using a VTK wasm module per viewport.
- [WrappedMace](./WrappedMace/README.md) Shows how to write a 3D object viewer in JavaScript using wrapped VTK C++ classes.
