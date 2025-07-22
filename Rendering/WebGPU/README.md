# VTK::RenderingWebGPU

## vtkRenderingWebGPU - WebGPU Backend for Rendering

### Description

This module contains the WebGPU native backend for `RenderingCore`. Currently, it supports rendering polygonal geometry in different representations with point/cell scalar-mapped colors.

When both the `RenderingOpenGL2` and `RenderingWebGPU` libraries are linked, the user must set the
`VTK_GRAPHICS_BACKEND` environment variable to either `WEBGPU` or `OPENGL` in order to activate
the intended object factories. In the future, we plan to enhance the object factory mechanism to accept command
line arguments.

---

### Building VTK with Dawn (Highly Experimental)

#### Prerequisites

- Git
- tools for building VTK

This module uses Dawn's C++ WebGPU implementation on desktop and the `emdawnwebgpu` subcomponent for wasm. You can either build Dawn from scratch or download pre-built releases for your machine from [kitware:utils/ci-utilities/dawn-v7037-20250226.0](https://gitlab.kitware.com/utils/ci-utilities/-/releases/dawn%2Fv7037-20250226.0)

#### Cloning and Building Dawn

Dawn should be built at tag [chromium/7037](https://dawn.googlesource.com/dawn.git/+show/chromium/7037).

```sh
# Clone the repo and checkout the required version
git clone https://dawn.googlesource.com/dawn dawn && cd dawn
git checkout chromium/7037
```

##### Build Dawn with `CMake` and `Ninja`

```sh
cmake -S . -B out/Debug -GNinja -DDAWN_FETCH_DEPENDENCIES=ON -DDAWN_ENABLE_INSTALL=ON
cmake --build out/Debug
cmake --install out/Debug --prefix /path/to/install/dawn
```

#### Configuring and Building VTK

When configuring VTK's build for wasm, configure with:

```sh
cmake \
-S /path/to/vtk/src \
-B /path/to/vtk/build \
-GNinja \
-DVTK_ENABLE_WEBGPU=ON \
-Demdawnwebgpu_DIR=/path/to/where/dawn/is/installed/lib/cmake/emdawnwebgpu \
-DVTK_BUILD_TESTING=ON

cmake --build
```

When building on desktop (x86_64/aarch64), configure with:

```sh
cmake \
-S /path/to/vtk/src \
-B /path/to/vtk/build \
-GNinja \
-DVTK_ENABLE_WEBGPU=ON \
-DDawn_DIR=/path/to/where/dawn/is/installed/lib/cmake/Dawn \
-DVTK_BUILD_TESTING=ON

cmake --build
```

---

### Running Tests

#### WebGPU Tests

```sh
./bin/vtkRenderingWebGPUCxxTests
```

Available tests:

```
0. TestActorFaceCullingProperty
1. TestAxesActor
2. TestCellScalarMappedColors
3. TestCompositePolyDataMapper
..
..
```

#### Rendering Core Tests

To run the `RenderingCore` tests with `VTK::RenderingWebGPU`, edit `vtk.module` to link unit tests with `TEST_DEPENDS`:

1. Uncomment the module name under `TEST_DEPENDS`.
2. Rebuild and run the tests (only a few pass currently).
3. Set the `VTK_GRAPHICS_BACKEND` environment variable to `WEBGPU`

```sh
export VTK_GRAPHICS_BACKEND=WEBGPU
./bin/vtkRenderingCoreCxxTests
```

---

### Features

The following features are currently implemented:

- **vtkPolyData Rendering**: Supports point, line, and triangle primitives.
- **Glyph Rendering**: Supports point, line, and triangle primitives.
- **Composite vtkPolyData Rendering**: Supports point, line, and triangle primitives.
- **Scalar Mapped Coloring**:
  - Point scalar mapped coloring of surfaces.
  - Cell scalar mapped coloring.
- **Actor Representations**:
  - `VTK_POINTS`
  - `VTK_WIREFRAME`
  - `VTK_SURFACE`
  - `VTK_SURFACE` with edge visibility
- **Lighting**:
  - Based on VTK headlights.
  - Supports point/cell normals.
- **Rendering Adjustments**:
  - Point size adjustments.
  - Line width adjustments for wireframe and surface with edges.
- **Rendering Backends**:
  - `vtkWebAssemblyWebGPURenderWindow`: A reference implementation of `vtkWebGPURenderWindow` for WebAssembly and desktop.
  - `vtkXWebGPURenderWindow`: Implementation using X11 for Linux desktop rendering.
- **Depth Testing**: Enables correct rendering of 3D objects.
- **Selections**: Hardware selector can pick cells, composite datasets and actors.
---

### Compute Shader API

The compute shader API allows offloading work from the CPU to the GPU using WebGPU compute shaders.

- **User-level information**: [Compute API User Guide](./doc/webgpu-compute-api-user.md)
- **Developer-level information**: [Compute API Developer Guide](./doc/webgpu-compute-api-dev.md)

---

### Future Work

Since WebGPU is already an abstraction over graphics APIs, this module avoids creating another level of abstraction. It leverages WebGPU's C++ flavor for its object-oriented API and RAII. Helper classes in the `vtkWebGPUInternals...` files ensure cleaner bind group initialization code.

Planned improvements include:

- Volume mappers
- Textures
- Dual-depth peeling
- Advanced lighting
- Platform-native render windows for Windows, macOS, Android, iOS and wayland.

---

### References

Here are some valuable resources for learning WebGPU:

1. **[WebGPU Fundamentals](https://webgpufundamentals.org/)**

   - Complete introduction to using WebGPU

2. **[WebGPU GLTF Case Study](https://toji.github.io/webgpu-gltf-case-study/)**

   - Builds an efficient glTF renderer in WebGPU using JavaScript.
   - Discusses various rendering pitfalls and optimizations.

3. **[WebGPU Native Examples](https://github.com/samdauwe/webgpu-native-examples/)**

   - A collection of single-file examples demonstrating various WebGPU use cases in C.

4. **[Learn WebGPU](https://eliemichel.github.io/LearnWebGPU/index.html)**

   - Similar to LearnOpenGL and Vulkan tutorials.
   - Covers window setup, triangle rendering, buffers, textures, and 3D rendering.

5. **[Learn WGPU](https://sotrh.github.io/learn-wgpu/)**

   - Beginner-friendly tutorial using wgpu.rs.

6. **[Raw WebGPU](https://alain.xyz/blog/raw-webgpu)**

   - Introductory tutorial covering WebGPU concepts using JavaScript.

7. **[How to Render a WebGPU Triangle (Series)](https://carmencincotti.com/2022-12-19/how-to-render-a-webgpu-triangle-series-part-three-video/)**

   - Explains swapchain and image presentation in detail.

8. **[WebGPU Rocks](https://webgpu.rocks/)**

   - A well-organized WebGPU API and WGSL summary.

For WGSL specification, refer to: [WGSL Spec](https://www.w3.org/TR/WGSL/)
