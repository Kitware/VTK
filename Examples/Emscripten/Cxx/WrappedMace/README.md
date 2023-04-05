An example of wrapping vtk classes into JS using embind.

```
docker run --rm --entrypoint /bin/bash -v $PWD:/work -p 8000:8000 -it dockcross/web-wasm:20230222-162287d

cd /work/build-example

emcmake cmake \
  -G Ninja \
  -DVTK_DIR=/work/build-vtk-wasm \
  /work/src/Examples/Emscripten/Cxx/WrappedMace

cmake --build .
```

once built and running you can access the vtk instances for example ```vtk.instances.sphere.SetThetaResolution(22);```
