## Single WebAssembly binary for synchronous and asynchronous sessions

VTK now ships a single `vtkWebAssembly.wasm` binary that exposes both the
synchronous and asynchronous JavaScript APIs. The `VTK::WebAssemblyAsync` module
has been removed and its functionality merged into `VTK::WebAssembly`.

You no longer need to choose between two binaries to access WebGPU support: link
`VTK::RenderingWebGPU` as an optional dependency of your `WebAssembly` module and
build a single `vtkWebAssembly.wasm` that provides `vtkRemoteSession`,
`vtkStandaloneSession`, and both the `invoke` and `invokeAsync` entry points.

If your project declares a custom `WebAssemblyAsync` module, fold its
dependencies into `WebAssembly/vtk.module` and drop the async module.
