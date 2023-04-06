# WebAssembly Cone Example

This example aims to provide a base example on how to write a VTK viewer for
WebAssembly with both OpenGL and WebGPU backends. It shows how an application can
use a single wasm binary to serve using both the backends. In this example, the UI is very minimal.

The additional steps required are:

1. Update `CMakeLists.txt` so that the `VTK::RenderingWebGPU` component is requested when finding VTK.
2. Link your targets with `RenderingWebGPU`. There is no problem linking to both `RenderingWebGPU` and `RenderingOpenGL2`.
3. Preload environment variable with `VTK_GRAPHICS_BACKEND` set to 'WEBGPU'.

## Compiling example against VTK

We assume inside the `work/` directory to find the source of VTK under `src/`
and its build tree under `build-vtk-wasm`.

If VTK is not built yet, please follow the guide `../README.md`.

Let's create the build directory for our example

```
mkdir -p work/build
```

Start docker inside that working directory

```
docker run --rm --entrypoint /bin/bash -v $PWD:/work -p 8000:8000 -it dockcross/web-wasm:20230222-162287d

cd /work/build

emcmake cmake \
  -G Ninja \
  -DVTK_DIR=/work/build-vtk-wasm \
  /work/src/Examples/Emscripten/Cxx/Cone

cmake --build .
```

## Serve and test generated code

```
cd work/build
python3 -m http.server 8000
```

Open your browser to http://localhost:8000 and click on a backend.
