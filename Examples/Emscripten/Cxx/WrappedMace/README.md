An example of wrapping vtk classes into JS using embind.

```
docker run --rm --entrypoint /bin/bash -v $PWD:/work -it dockcross/web-wasm:20200416-a6b6635

cd /work/build-example

cmake \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} \
  -DVTK_DIR=/work/build-vtk-wasm \
  /work/src/Examples/Emscripten/Cxx/WrappedMace

cmake --build .
```

once built and running you can access the vtk instances for example ```vtk.instances.sphere.SetThetaResolution(22);```
