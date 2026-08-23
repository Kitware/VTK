# WebGPU Headers

The standard WebGPU C API header that VTK's WebGPU rendering module compiles
against.

## Source

Vendored from upstream
[webgpu-headers](https://github.com/webgpu-native/webgpu-headers), revision
`b3f67b89929c133403fd95638be4ef96b56ddca0`.

This is the implementation-neutral definition of the native WebGPU API. VTK
compiles against it on every platform, including builds that link Dawn, so no
implementation can change the API VTK is built against underneath it.

The revision is deliberately the one Dawn vendors for the release pinned by
`.gitlab/ci/download_dawn.cmake`, so the header VTK compiles against and the
implementation CI links describe the same API. Prefer keeping the two in step
over tracking the upstream tip.

## Headers

- `webgpu/webgpu.h` — the WebGPU C API

No C++ wrapper (`webgpu_cpp.h`) is vendored. VTK uses the C API only and
expresses handle ownership with `Rendering/WebGPU/Private/vtkWebGPUHandle.h`,
so the module builds as C++17.

## VTK Notes

Dawn (or another conformant WebGPU implementation) is still required at link
time for runtime WebGPU functionality; only the headers are vendored here.

Upstream deliberately declares only the standard API. Implementations extend it
through `nextInChain`, using `WGPUSType` values from blocks upstream reserves
for them. The few extensions VTK uses are declared in
`Rendering/WebGPU/Private/vtkWebGPUImplExtensions.h`.

To update: copy `webgpu.h` and `LICENSE` from a checkout of the upstream
repository into `vtkwebgpuheaders/`, and update the `VERSION` in the outer
`CMakeLists.txt` to the upstream revision. Confirm the new header is still a
superset-compatible match for the implementation being linked - compare struct
member layouts and enum values against that implementation's own `webgpu.h`
before assuming an update is safe.
