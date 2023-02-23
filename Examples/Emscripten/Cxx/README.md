# Building WebAssembly examples with rendering

This example aims to provide a base example on how to create an example using
VTK for its rendering while running the code inside a browser using WebAssembly.

This example was contributed by Rostyslav Lyulinetskyy and Ilya Volkov from
https://about.dicehub.com/.

## Compiling VTK for Emscripten

Please refer to [Documentation/dev/build_wasm_emscripten.md](https://gitlab.kitware.com/vtk/-/blob/master/Documentation/dev/build_wasm_emscripten.md) for
detailed instructions.

## Compiling all Emscripten examples

```
docker run --rm --entrypoint /bin/bash -v $PWD:/work -p 8000:8000 -it dockcross/web-wasm:20230222-162287d

mkdir -p /work/build-examples-wasm
cd /work/build-examples-wasm

cmake \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} \
  -DVTK_DIR=/work/build-vtk-wasm \
  /work/src/Examples

cmake --build .
```

## Serve and test generated code

```
cd work/build-examples-wasm/Emscripten/Cxx/
python3 -m http.server 8000
```

Open your browser to:

- http://localhost:8000/Cone
- http://localhost:8000/ConeFullScreen
- http://localhost:8000/MultiCone
- http://localhost:8000/WrappedMace
