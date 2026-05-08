# VTK::RenderingWebGPU

## Description

This module contains the WebGPU native backend for `RenderingCore`. Currently, it supports rendering polygonal geometry in different representations with point/cell scalar-mapped colors.

When both the `RenderingOpenGL2` and `RenderingWebGPU` libraries are linked, the user must provide preferences to select the rendering backend at runtime. This can be done in two ways:
1. Pass command line arguments to your application `--vtk-factory-prefer RenderingBackend=WebGPU` or `--vtk-factory-prefer RenderingBackend=OpenGL2`. Then, you will need to invoke `vtkObjectFactory::InitializePreferencesFromCommandLineArgs(argc, argv)` in your main function before creating any VTK object.
2. Alternatively, set the `VTK_FACTORY_PREFER` environment variable to `RenderingBackend=WebGPU` or `RenderingBackend=OpenGL2`.

---

## Building VTK with Dawn (Highly Experimental)

### Prerequisites

- Git
- tools for building VTK

### Desktop
On desktop (Linux, macOS, and Windows), this module uses Dawn's C++ WebGPU implementation. You can get Dawn with any of these two methods:

1. Build Dawn from source.
2. Fetch pre-built Dawn binaries built in release mode.

These two methods are supported on all three operating systems. Pre-build Dawn binaries are available only
for linux-x86_64, windows-x86_64, macos-x86_64 and macos-arm64. If you are on a different platform, you will need to build Dawn from source.

Here, `VTK_SOURCE_DIR` is the path to the root of the VTK source directory, and `VTK_BUILD_DIR` is the path to the directory where you want to build VTK.

#### Build Dawn from source

Dawn should be built at tag [v20260421.125655](https://github.com/google/dawn/tree/v20260421.125655).
Here, `DAWN_INSTALL_DIR` should point to the directory where Dawn is installed (should contain `lib` and `include` directories).

```sh
# Clone the repo and checkout the required version
git clone https://github.com/google/dawn dawn && cd dawn
git checkout v20260421.125655
cmake -S . -B out/Debug -GNinja -DDAWN_FETCH_DEPENDENCIES=ON -DDAWN_ENABLE_INSTALL=ON
cmake --build out/Debug
cmake --install out/Debug --prefix ${DAWN_INSTALL_DIR}
```

#### Fetch pre-built Dawn binaries

Run the following command to fetch pre-built Dawn binaries. This will download and install Dawn to `VTK_SOURCE_DIR/.gitlab`.

::::{tab-set}

:::{tab-item} Linux
```sh
cd "${VTK_SOURCE_DIR}"
CMAKE_CONFIGURATION="fedora" cmake -P .gitlab/ci/download_dawn.cmake
export DAWN_INSTALL_DIR="${VTK_SOURCE_DIR}/.gitlab/dawn"
```
:::

:::{tab-item} macOS-arm64
```sh
cd "${VTK_SOURCE_DIR}"
CMAKE_CONFIGURATION="macos_arm64" cmake -P .gitlab/ci/download_dawn.cmake
export DAWN_INSTALL_DIR="${VTK_SOURCE_DIR}/.gitlab/dawn"
```
:::

:::{tab-item} macOS-x86_64
```sh
cd "${VTK_SOURCE_DIR}"
CMAKE_CONFIGURATION="macos_x86_64" cmake -P .gitlab/ci/download_dawn.cmake
export DAWN_INSTALL_DIR="${VTK_SOURCE_DIR}/.gitlab/dawn"
```
:::

:::{tab-item} Windows
```pwsh
cd "$env:VTK_SOURCE_DIR"
$env:CMAKE_CONFIGURATION="windows"; cmake -P .gitlab/ci/download_dawn.cmake
$env:DAWN_INSTALL_DIR="$env:VTK_SOURCE_DIR\.gitlab\dawn"
```
:::
::::

After you are finished, you should see a new directory `VTK_SOURCE_DIR/.gitlab/dawn` which contains the pre-built Dawn binaries. Set `DAWN_INSTALL_DIR` to `VTK_SOURCE_DIR/.gitlab/dawn` so that you can pass it to CMake when configuring VTK's build.

#### Configure and build VTK

Run the following commands to configure and build VTK with WebGPU support.

::::{tab-set}
:::{tab-item} Linux/macOS
```sh
cmake \
-S "${VTK_SOURCE_DIR}" \
-B "${VTK_BUILD_DIR}" \
-GNinja \
-DVTK_ENABLE_WEBGPU=ON \
-DDawn_DIR="${DAWN_INSTALL_DIR}/lib/cmake/Dawn" \
-DVTK_BUILD_TESTING=ON

cmake --build "${VTK_BUILD_DIR}"
```
:::

:::{tab-item} Windows
```pwsh
cmake `
-S "$env:VTK_SOURCE_DIR" `
-B "$env:VTK_BUILD_DIR" `
-DCMAKE_BUILD_TYPE=Release `
-GNinja `
-DVTK_ENABLE_WEBGPU=ON `
-DDawn_DIR="$env:DAWN_INSTALL_DIR/lib/cmake/Dawn" `
-DVTK_BUILD_TESTING=ON

cmake --build "$env:VTK_BUILD_DIR"
```
:::
::::

```{warning}
When copy pasting the commands on Windows, please paste them into a Powershell window and not a Command prompt!
```

```{warning}
On Windows, ensure that you are using the same `CMAKE_BUILD_TYPE` for both building Dawn and configuring VTK. Or else, you will see a linker error `LNK2038: mismatch detected for '_ITERATOR_DEBUG_LEVEL': value '0' doesn't match value '2' in vtkRenderingWebGPUObjectFactory.cxx.obj`.
```

### WebAssembly
On WebAssembly, this module uses the WebGPU implementation provided by the browser. You do not need to build Dawn from source or fetch pre-built Dawn binaries. You just need to configure and build VTK with Emscripten. It takes care of linking the WebGPU implementation provided by the browser (with the `--use-port=emdawnwebgpu` flag).

#### Configure and build VTK

Run the following commands to configure and build VTK for WASM with WebGPU support.

```sh
emcmake cmake \
-S "${VTK_SOURCE_DIR}" \
-B "${VTK_BUILD_DIR}" \
-GNinja \
-DVTK_ENABLE_WEBGPU=ON \
-DBUILD_SHARED_LIBS=OFF \
-DVTK_BUILD_TESTING=ON

cmake --build "${VTK_BUILD_DIR}"
```
---

## Running Tests

### WebGPU Tests

::::{tab-set}
:::{tab-item} Linux/macOS
```sh
cd ${VTK_BUILD_DIR}
ctest -R RenderingWebGPU -V
```
:::

:::{tab-item} Windows
```pwsh
cd $env:VTK_BUILD_DIR
ctest -R RenderingWebGPU -V
```
:::
::::

### Rendering Core Tests

To run the `RenderingCore` tests with `VTK::RenderingWebGPU`:

::::{tab-set}
:::{tab-item} Linux/macOS
```sh
cd ${VTK_BUILD_DIR}
ctest -R RenderingCoreCxx-WebGPU -V
```
:::

:::{tab-item} Windows
```pwsh
cd $env:VTK_BUILD_DIR
ctest -R RenderingCoreCxx-WebGPU -V
```
:::
::::

---

## Features

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
- **Depth Testing**: Enables correct rendering of 3D objects.
- **Selections**: Hardware selector can pick cells, composite datasets and actors.
---

## Compute Shader API

The compute shader API allows offloading work from the CPU to the GPU using WebGPU compute shaders.

- **User-level information**: [Compute API User Guide](./doc/webgpu-compute-api-user.md)
- **Developer-level information**: [Compute API Developer Guide](./doc/webgpu-compute-api-dev.md)

---

## Future Work

Since WebGPU is already an abstraction over graphics APIs, this module avoids creating another level of abstraction. It leverages WebGPU's C++ flavor for its object-oriented API and RAII. Helper classes in the `vtkWebGPUInternals...` files ensure cleaner bind group initialization code.

Planned improvements include:

- Volume mappers
- Textures
- Dual-depth peeling
- Advanced lighting
- Platform-native render windows for Windows, macOS, Android, iOS and wayland.

---

## References

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
