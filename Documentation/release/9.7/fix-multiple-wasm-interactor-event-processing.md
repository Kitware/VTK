## WebAssembly interactor event loops can now coexist

VTK now allows multiple interactor event loops to run simultaneously in WebAssembly environments. Previously, the implementation used `emscripten_set_main_loop_arg`, which prevented simultaneous event loop execution. The new design enables this capability by adopting the approach used in the async code path, making it easier to work with multiple interactors in WASM applications.
