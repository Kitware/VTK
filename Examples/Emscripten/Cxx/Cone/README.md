# WebAssembly Cone Example

This example aims to provide a base example on how to write a VTK viewer for
WebAssembly.

## Compiling example against VTK

We assume inside the `work/` directory to find the source of VTK under `src/`
and its build tree under `build-vtk-wasm`.

If VTK is not built yet, please follow the guide `../README.md`.

Let's create the build directory for our example

```
mkdir -p work/build-cone
```

Start docker inside that working directory

```
docker run --rm --entrypoint /bin/bash -v $PWD:/work -p 8000:8000 -it dockcross/web-wasm:20230222-162287d

cd /work/build-cone

emcmake cmake \
  -G Ninja \
  -DVTK_DIR=/work/build-vtk-wasm \
  /work/src/Examples/Emscripten/Cxx/Cone

cmake --build .
```

## Serve and test generated code

```
cd work/build-cone
python3 -m http.server 8000
```

Open your browser to http://localhost:8000
