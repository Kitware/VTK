# Query WebAssembly heap memory usage from JavaScript

The `vtkWebAssembly.wasm` binary now exposes a `getMemoryUsageStatistics()` function that reports global WebAssembly heap memory usage. It returns a JavaScript object with all values in bytes:

- `free`: memory available for future allocations.
- `max`: the size the heap can grow to. This is the limit that applies when memory growth is allowed, which `VTK::WebAssembly` enables by default with `-sALLOW_MEMORY_GROWTH=1`.
- `total`: `free + used`.
- `used`: memory currently held by allocated objects, static data, and the stack.

Use the statistics to monitor memory pressure in a VTK WebAssembly application, for example to display a usage meter or to decide when to release large datasets or call `wasmModule._free` on cached buffers.
