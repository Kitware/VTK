# Testing wasm example

This example demonstrates how to use VTK testing infrastructure to run unit tests in your own modules that are built using the VTK CMake module
APIs

## Build

```
emcmake cmake -S . -B out -GNinja -DVTK_DIR="/path/to/vtk/install/lib/cmake/vtk-9.4" -DVTK_TESTING_WASM_ENGINE:FILEPATH=/path/to/chrome/chrome
```

## Run
```
cd out
ctest
```
