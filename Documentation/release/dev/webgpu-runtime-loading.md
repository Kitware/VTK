# WebGPU Runtime Loading

VTK's WebGPU rendering module now resolves a WebGPU implementation through a
proc table that is loaded when the WebGPU configuration is initialized. If no
implementation is found, initialization fails with a diagnostic so applications
can fall back to another backend such as OpenGL.

The goal is to let the same VTK binary run against different native
implementations (Dawn, wgpu-native) or the browser's WebGPU via Emscripten. That
goal is not reached yet: when Dawn is found at configure time the module still
links it, so the proc table is currently an additional runtime requirement
rather than a replacement for the link-time dependency.

VTK's public headers now use the WebGPU C API only, so the installed interface
no longer exposes Dawn-specific types and does not require C++20 of its
consumers. Applications that use VTK's rendering classes are unaffected; code
that referenced VTK's WebGPU internals directly may need to be updated for the
C API. Internally the module still uses Dawn's C++ `wgpu::` types, and the
vendored headers are still taken from Dawn rather than from upstream
`webgpu-headers`; completing both migrations is follow-up work.

## Notes

- Library discovery follows the usual search paths (`LD_LIBRARY_PATH` on Linux,
  `DYLD_LIBRARY_PATH` on macOS).
- Windows is not supported yet: the proc table is POSIX-only (`dlopen`/`dlsym`),
  and a `LoadLibrary`/`GetProcAddress` path is pending.
- Where a function is unavailable in a given runtime, the wrappers return safe
  defaults so optional features can be disabled gracefully.
