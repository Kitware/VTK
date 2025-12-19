# Add WebAssembly interface for standalone applications

You can now develop remote wasm or standalone wasm applications using the `vtkWebAssembly.wasm` binary.
The `VTK::WebAssembly` and `VTK::WebAssemblyAsync` now builds a `vtkWebAssembly[Async].wasm` and associated
`.mjs` file that provide `vtkRemoteSession` and `vtkStandaloneSession` JavaScript classes.

- Remote session API is concerned with use cases where a "server" creates VTK
 objects and sends the state to a WASM "client" that deserializes the state
 into objects to mimic the visualization pipeline on the "server".
 This API does not allow creating objects in the WASM world. It is possible,
 although very difficult and prone to bugs.

- Standalone API is important when one wants to directly create and manipulate objects
 in the local context in the absence of a server.
