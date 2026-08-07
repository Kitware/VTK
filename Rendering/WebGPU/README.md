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

Dawn should be built at tag [v20260720.160313](https://github.com/google/dawn/tree/v20260720.160313).
Here, `DAWN_INSTALL_DIR` should point to the directory where Dawn is installed (should contain `lib` and `include` directories).

```sh
# Clone the repo and checkout the required version
git clone https://github.com/google/dawn dawn && cd dawn
git checkout v20260720.160313
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

---

## WebGPU Runtime Loading

VTK's WebGPU module resolves a WebGPU implementation library at runtime through a
proc table. The eventual goal is to decouple the compile-time and runtime
dependencies so that the same VTK build works with different native WebGPU
runtimes (Dawn, wgpu-native) or browser WebGPU via Emscripten.

```{note}
This decoupling is not complete. When Dawn is found at configure time the module
still links `dawn::webgpu_dawn` (see `VTK::WebGPUImpl` in `CMakeLists.txt`), and
`vtkWebGPUConfiguration::Initialize()` fails if the proc table cannot load an
implementation. Today the proc table is therefore an *additional* runtime
requirement layered on the link-time dependency, not a replacement for it.
```

The implementation uses *lazy initialization* that is thread-safe and loads on
first access. It uses `RTLD_GLOBAL` so existing `wgpu::` C++ calls work without
modification. Missing libraries are reported at initialization (not link time),
with clear diagnostic messages. If functions are unavailable, wrappers return
safe values for graceful fallback.

### Architecture

The implementation consists of three layers:

1. **vtkWebGPUProcTable** (C interface): Low-level dlopen/dlsym wrapper that
   loads the WebGPU implementation library and resolves function pointers using
   the runtime's proc address function.

2. **vtkWebGPUProcLoader** (C++ RAII singleton, internal): Wraps the proc table
   with lazy initialization. Provides `IsLoaded()` and `Load()` methods and
   handles cleanup on shutdown. Accessed indirectly via
   `vtkWebGPUConfiguration::Initialize()`.

3. **vtkWebGPUProcAPI** (C convenience wrappers): Thin wrappers for
   frequently-used WebGPU functions. Existing `wgpu::` C++ code continues to
   work unchanged because the library is loaded with global symbol visibility
   (`RTLD_GLOBAL`).

Initialization flow: `vtkWebGPUConfiguration::Initialize()` invokes
`vtkWebGPUProcLoader::GetInstance()`, which loads the WebGPU implementation
library (e.g., `libwgpu_dawn.so`), resolves function pointers via the proc
table, then creates a WebGPU adapter and device.

### Standard WebGPU Headers

VTK vendors a copy of the WebGPU headers in `ThirdParty/webgpuheaders`. They
define the core WebGPU types (`WGPUInstance`, `WGPUDevice`, etc.) and the
function signatures that the proc table resolves at runtime. They are used when
no Dawn installation is found at configure time, and for Emscripten builds; when
Dawn *is* found, its own headers are used instead.

VTK's public headers already use the WebGPU C API only, so the installed
interface does not expose `wgpu::` types and does not require C++20 of its
consumers.

```{note}
The vendored headers are currently taken from Dawn, not from upstream
[webgpu-headers](https://github.com/webgpu-native/webgpu-headers) —
`include/webgpu/webgpu_cpp.h` is a shim that includes `include/dawn/webgpu_cpp.h`.
So VTK is not yet decoupled from a particular implementation at the header level.
Two changes are needed to get there: vendoring the upstream C headers, and
removing `webgpu_cpp.h` from the module's implementation files, which still use
`wgpu::` types internally.
```

### Library Search Strategy

If an explicit library path is given to the loader, it is tried first and on its
own. Otherwise the proc table tries the following names, in this order:

1. `libwgpu_dawn.so` (the default when no path is supplied)
2. `libwebgpu_dawn.so`
3. `libwgpu_dawn.so.0` (versioned variant)
4. `libwebgpu_dawn.so.0` (versioned variant)
5. `libwgpu_dawn.dylib` (macOS)
6. `libwebgpu_dawn.dylib` (macOS)
7. `wgpu_dawn.dll` (Windows — see the note below)
8. `webgpu_dawn.dll` (Windows — see the note below)

```{warning}
**Windows is not supported yet.** `vtkWebGPUProcTable.cxx` is POSIX-only: it
includes `<dlfcn.h>` and calls `dlopen`/`dlsym` with no `_WIN32` branch, so it
does not compile with MSVC and the two `.dll` entries above are unreachable. A
`LoadLibrary`/`GetProcAddress` path is pending. Until it lands, the Windows
build instructions earlier in this document will not work, and the
`windows-vs2022-webgpu` CI jobs are disabled.
```

**Custom paths**: You can override the search by setting environment variables:
- Linux: `LD_LIBRARY_PATH=/path/to/lib`
- macOS: `DYLD_LIBRARY_PATH=/path/to/lib`
- Windows: `PATH=\path\to\lib` (once the Windows path above is implemented)

Or by explicitly passing a path to the library loader.

### Extending the API

If you need to expose an additional WebGPU function through the C wrappers:

1. Add a declaration in `vtkWebGPUProcAPI.h` with the appropriate export macro.
2. Implement a thin wrapper in `vtkWebGPUProcAPI.cxx` that:
   - Retrieves the proc table
   - Looks up the function pointer by name (with matching length)
   - Checks for NULL and returns a safe value if not found
   - Forwards the call with its arguments
3. Add a small test to verify the function resolves at runtime.

```{warning}
The length in the `WGPUStringView` must equal the length of the name in bytes.
Getting it wrong truncates the symbol and the lookup silently fails at runtime,
returning the wrapper's fallback value rather than reporting an error. Prefer
`WGPU_STRLEN` (or `strlen`) over a hand-counted literal.
```

Example:

```cpp
// In vtkWebGPUProcAPI.cxx
WGPUReturnType vtkWebGPUMyFunction(WGPUArgumentType arg)
{
  vtkWebGPUProcTable table = vtkWebGPUProcTableGet();
  if (!table)
    return NULL;

  typedef WGPUReturnType (*FuncType)(WGPUArgumentType);
  FuncType func = (FuncType)vtkWebGPUProcTableGetProc(
      table, WGPUStringView{ "wgpuMyFunction", WGPU_STRLEN });

  if (!func)
    return NULL;

  return func(arg);
}
```

### Debugging and Diagnostics

**Library loading failed:**
- Check if the WebGPU library is installed: `ldconfig -p | grep -E
  '(wgpu_dawn|webgpu_dawn)'` (Linux), or verify in `/usr/lib` or
  `/opt/local/lib` (macOS), or `PATH` (Windows)
- Ensure the library is in a standard search path or set
  `LD_LIBRARY_PATH=/path/to/lib` (Linux), `DYLD_LIBRARY_PATH` (macOS), or
  `PATH` (Windows)
- Test library loading directly: `python3 -c "import ctypes;
  ctypes.CDLL('/path/to/libwgpu_dawn.so')"` (adjust path/name as needed)

**Function not found:**
- Verify the function name is spelled correctly and the length is correct (must
  match string length in bytes)
- Check if the loaded library version exports the function: `nm
  /usr/lib/libwgpu_dawn.so | grep wgpuFunctionName`
- Enable debug output in vtkWebGPUProcTable.cxx during development

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

Since WebGPU is already an abstraction over graphics APIs, this module avoids creating another level of abstraction. Helper classes in the `Private/vtkWebGPU<Thing>Internals` files ensure cleaner bind group initialization code.

The module currently uses Dawn's C++ `wgpu::` types internally for their
object-oriented API and RAII. Replacing them with the WebGPU C API plus VTK's own
`Private/vtkWebGPUHandle.h` reference-counted wrapper is in progress; the public
headers have already been converted.

Planned improvements include:

- Volume mappers
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
