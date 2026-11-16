## Compile Slang shaders with SlangKit

The new `VTK::RenderingSlang` module compiles [Slang](https://shader-slang.org) shaders to WGSL
through the [SlangKit](https://github.com/sankhesh/SlangKit) library. `vtkSlangShaderManager` takes
defines, modules, search paths, and specializations by interface name, reports the bindings of the
compiled code, and plans and packs the values of uniform blocks from reflection. The module is built
with the WebGPU backend; set `VTK_USE_SLANG` and point `SlangKit_DIR` at SlangKit 0.1 or newer to
compile, otherwise the manager reports that Slang is unavailable.
