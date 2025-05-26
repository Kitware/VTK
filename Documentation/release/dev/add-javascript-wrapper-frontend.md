## Add JavaScript wrappers with Emscripten

Use the automated wrapping infrastructure of the VTK::WrappingTools module to generate JavaScript bindings of the VTK C++ classes.
Enable the VTK_WRAP_JAVASCRIPT CMake option to generate a single wasm file exposing the VTK API in JavaScript. See Examples/JavaScript for usage.
See `vtkModuleWrapJavaScriptExclusions.cmake` to exclude specific modules and classes from the wrapping process.
