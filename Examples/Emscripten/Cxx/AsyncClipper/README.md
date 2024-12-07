# Build

This example needs VTK.wasm compiled with `VTK_WEBASSEMBLY_THREADS=ON`. If you're compiling
with vtk-wasm-sdk image from dockerhub, make sure to use the threads variant.

```
emcmake cmake \
  -G Ninja \
  -S /path/to/vtk/Examples/Emscripten/Cxx/Cone \
  -B out/build \
  -DVTK_DIR=/path/to/where/vtk/wasm/was/built

cmake --build out/build
```

# Serve and test generated code

This example uses wasm in web workers and needs `SharedArrayBuffer`.
For that reason, use the `server.py` from this directory which configures the required
HTTP headers for browsers to allow `SharedArrayBuffer`.

```
cd out/build
python3 ./server.py
```

Open your browser to http://localhost:8000
