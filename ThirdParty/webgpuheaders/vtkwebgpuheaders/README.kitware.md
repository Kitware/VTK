# WebGPU Headers

These headers provide the standard WebGPU C/C++ API used by VTK's WebGPU rendering module.

## Source

Vendored from Dawn (https://dawn.googlesource.com/dawn), which implements the
[WebGPU standard](https://www.w3.org/TR/webgpu/) and the
[webgpu-headers](https://github.com/webgpu-native/webgpu-headers) native API.

Taken from Dawn `v20260720.160313`, the same release pinned by
`.gitlab/ci/download_dawn.cmake`, so the vendored headers and the runtime CI
downloads describe the same API. Tags of that form are published on the Dawn
mirror at https://github.com/google/dawn/tags.

## Headers

- `webgpu/webgpu.h` — WebGPU C API
- `webgpu/webgpu_cpp.h` — WebGPU C++ wrapper (wgpu:: namespace)
- `webgpu/webgpu_cpp_chained_struct.h` — Chained struct helpers
- `webgpu/webgpu_cpp_print.h` — Debug print helpers
- `webgpu/webgpu_enum_class_bitmasks.h` — Bitmask enum helpers

The `webgpu/` headers are redirect wrappers; the generated content lives in
`dawn/`.

## VTK Notes

These headers are the public interface for all `wgpu::` types used in VTK's WebGPU
public API. Dawn (or another conformant WebGPU implementation) is still required at
link time for runtime WebGPU functionality.

To update: copy the headers from a Dawn install into
`vtkwebgpuheaders/include/`, re-apply the patches listed above, and update the
`VERSION` in the outer `CMakeLists.txt` to match the Dawn release.
