This module contains the WebGPU native backend for `RenderingCore`. At the moment, only polygonal geometry can be rendered in different representations with point/cell scalar mapped colors.

Here is a list of currently implemented features:
1. Polygonal geometry rendering with point, line and triangle primitives.
2. Point scalar mapped coloring of surfaces.
3. Cell scalar mapped coloring.
4. Draw actors with the actor representation = `VTK_POINTS`, `VTK_WIREFRAME`, `VTK_SURFACE` and `VTK_SURFACE` with edge visibility.
5. Lighting based on VTK headlights and point/cell normals.
6. Point size adjustments.
7. Line width adjustments for wireframe and surface with edges.
8. `vtkSDL2WebGPURenderWindow` is a reference implementation of `vtkWebGPURenderWindow` that works on WebAssembly and desktop.
9. Depth testing.

Since WebGPU is already an abstraction over graphics APIs, this module doesn't create another level of abstraction. It uses WebGPU's C++ flavor
for it's object-oriented API and RAII. There are helper classes in the `vtkWebGPUInternals...` files for convenience and to make the bind group
initialization code look clean.

A lot of work remains to be done. Selections, volume mappers, textures, dual-depth peeling, fancy lights, platform native render windows are few that come to mind.


## How to build VTK with Dawn (Highly experimental)

This module uses Dawn-C++ WebGPU implementation when VTK is built outside emscripten. First grab [Dawn](https://dawn.googlesource.com/dawn/) and follow their
build instructions using `gn`, not CMake.

Dawn uses the Chromium build system and dependency management so you need to install [depot_tools](http://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up) and add it to the PATH.

```sh
# Clone the repo as "dawn"
git clone https://dawn.googlesource.com/dawn dawn && cd dawn

# Bootstrap the gclient configuration
cp scripts/standalone.gclient .gclient

# Fetch external dependencies and toolchains with gclient
gclient sync
```

### Build Dawn with `gn` and Ninja

```sh
mkdir -p out/Debug
gn gen out/Debug
autoninja -C out/Debug
```

### Configure and build VTK

```sh
$ cmake \
-S /path/to/vtk/src \
-B /path/to/vtk/build \
-GNinja \
-DVTK_ENABLE_WEBGPU=ON \
-DVTK_USE_SDL2=ON \
-DDAWN_SOURCE_DIR=/path/to/dawn/src \
-DDAWN_BINARY_DIR=/path/to/dawn/src/out/Debug

$ cmake --build
```

### Run the WebGPU tests
These are not regression tested with image comparisons.
```sh
$ export VTK_WINDOW_BACKEND=SDL2
$ ./bin/vtkRenderingWebGPUCxxTests
Available tests:
  0. TestCellScalarMappedColors
  1. TestConesBenchmark
  2. TestLineRendering
  3. TestPointScalarMappedColors
  4. TestSurfacePlusEdges
  5. TestTheQuad
  6. TestTheQuadPointRepresentation
  7. TestTheQuadWireframeRepresentation
  8. TestTheTriangle
  9. TestTheTrianglePointRepresentation
 10. TestTheTriangleWireframeRepresentation
 11. TestVertexRendering
 12. TestWireframe
```

### Run the Rendering Core tests
Very few of these pass.
```sh
$ export VTK_WINDOW_BACKEND=SDL2
$ export VTK_GRAPHICS_BACKEND=WEBGPU
$ ./bin/vtkRenderingCoreCxxTests
```
