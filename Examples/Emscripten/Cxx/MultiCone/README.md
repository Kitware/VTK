```
docker run --rm --entrypoint /bin/bash -v $PWD:/work -it dockcross/web-wasm:20230222-162287d

cd /work/build-example

emcmake cmake \
  -G Ninja \
  -DVTK_DIR=/work/build-vtk-wasm \
  /work/src/Examples/Emscripten/Cxx/MultiCone

cmake --build .
```
