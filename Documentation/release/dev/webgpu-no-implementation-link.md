# WebGPU: no implementation is linked

The WebGPU module no longer links Dawn, or any other implementation. Every
`wgpu*` entry point VTK calls is resolved at runtime through the proc table and
reached through a dispatch table, so `libvtkRenderingWebGPU` carries no
reference to an implementation and the same binary runs against whichever one is
present.

What this changes for a build:

- Dawn is still located at configure time when it is available, but only so that
  its implementation-specific extensions can be declared. A build without it
  produces the same library, and that library still runs on Dawn.
- The implementation is looked for at initialization, in this order:
  `VTK_WEBGPU_LIBRARY` if set, then the names implementations are installed
  under (searched through `LD_LIBRARY_PATH`, `DYLD_LIBRARY_PATH` or `PATH`), and
  finally the implementation the build was configured against.
- Emscripten is unchanged: `--use-port=emdawnwebgpu` links the implementation
  into the module, so the redirect is not applied and the calls bind normally.

`Rendering/WebGPU/Private/vtkWebGPUProcDispatch.{h,cxx}` are generated from the
vendored headers by `Rendering/WebGPU/generate_proc_dispatch.py`; rerun it when
those headers are updated.
