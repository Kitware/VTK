```
docker run --rm --entrypoint /bin/bash -v $PWD:/work -it dockcross/web-wasm:20230222-162287d

cd /work/build-example

cmake \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} \
  -DVTK_DIR=/work/build-vtk-wasm \
  /work/src/Examples/Emscripten/Cxx/MultiCone

cmake --build .
```
