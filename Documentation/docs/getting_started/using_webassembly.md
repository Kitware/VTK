# Using WebAssembly

VTK-Wasm is a prototype infrastructure that enables the compilation of VTK C++ code to WebAssembly via Emscripten. This feature is still under active development.

To learn more about VTK-Wasm and its capabilities, please take a look at the following resources:

* [Homepage for VTK.wasm](https://kitware.github.io/vtk-wasm/)

* [Examples of WebAssembly applications that use VTK for rendering.](https://gitlab.kitware.com/vtk/vtk/-/tree/master/Examples/Emscripten/Cxx)

* [A collection of VTK WebAssembly demos.](https://github.com/Kitware/vtk-wasm-demos)

* [A guide on using the experimental WebGPU feature in VTK-Wasm.](https://discourse.vtk.org/t/guide-how-do-i-use-vtk-wasm-webgpu-experimental-feature/11164).

* [Instructions for building VTK using Emscripten for WebAssembly.](../advanced/build_wasm_emscripten.md)

* [vtk-wasm-sdk](https://gitlab.kitware.com/vtk/vtk-wasm-sdk) for building and publishing the [`kitware/vtk-wasm-sdk`](https://hub.docker.com/r/kitware/vtk-wasm-sdk) docker images.

* [Deep dive into WebAssembly & WebGPU in VTK: presentation from April 28th, 2023](https://docs.google.com/presentation/d/1Nl0TVa55616QKCSHP54BoYBvByMKe6lIUl6IFZqSeJo/edit#slide=id.p). This presentation covers topics such as Emscripten, VTK-wasm Docker image, WASM Dev tools, VTK and WebGPU: PolyData Mapper, API inspection with RenderDoc, and performance profiles.

We welcome your feedback and contributions to this project. Feel free to share your experiences, questions, and ideas in the [web/vtk-wasm](https://discourse.vtk.org/c/web/vtk-wasm/12) category of the VTK Discourse forum. Stay tuned for updates and new developments!
