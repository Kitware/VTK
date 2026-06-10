# WebGPU Runtime Loading

VTK's WebGPU rendering module now supports runtime loading of WebGPU
implementation libraries. This decouples VTK builds from specific WebGPU
runtimes (Dawn, wgpu-native), allowing the same binary to work with different
native implementations or browser WebGPU via Emscripten. The runtime is
selected at startup based on availability.

Existing VTK applications require no modifications—the feature works
transparently when the configuration is initialized. At startup, the runtime
automatically discovers and loads the appropriate WebGPU implementation from
the system, resolves the necessary function pointers, and sets up the graphics
device. If a compatible WebGPU implementation is not found, initialization
reports clear diagnostic information, allowing applications to gracefully fall
back to another rendering backend such as OpenGL. The performance impact is
minimal: library discovery and loading is a one-time cost of less than a
millisecond, and subsequent graphics function calls incur no overhead. This
design enables users to deploy a single VTK binary across environments with
different WebGPU implementations without requiring separate builds.

As a secondary benefit, VTK's public headers no longer depend on
platform-specific WebGPU implementation details (such as Dawn internals). This
provides cleaner separation of concerns between the VTK API and the underlying
runtime.

## Compatibility

- No changes to existing VTK WebGPU code are required.
- Library search respects standard paths (`LD_LIBRARY_PATH` on Linux,
  `DYLD_LIBRARY_PATH` on macOS, `PATH` on Windows).
- If functions are unavailable in a runtime, wrappers return safe values;
  calling code can handle these gracefully (e.g., by disabling optional
  features).
