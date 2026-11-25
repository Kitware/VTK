# Customizing WebGPU shaders with Slang

This example renders a torus with `vtkWebGPUPolyDataMapper` and changes its
shading with Slang types instead of WGSL string replacements.
`custom_shading.slang` defines two of them:

- `Stripes`, an `IShaderHooks` type whose `color()` hook paints stripes over
  VTK's colors.
- `Toon`, an `ILightingModel` type that draws the diffuse lighting in flat bands.

`CustomShading.cxx` loads the module with
`vtkWebGPUShaderProperty::LoadSlangModule()`, adds its directory with
`AddSlangSearchPath()` so that its `import custom_palette` resolves, and selects
a type for an interface with `SetSlangSpecialization()`. Keys switch the torus
between:

| Key | Shading |
| --- | --- |
| `g` | the WGSL template |
| `d` | the Slang program with its default types, which renders like `g` |
| `h` | `Stripes` |
| `l` | `Toon` |
| `b` | both |

## Build SlangKit

[SlangKit](https://github.com/sankhesh/SlangKit) compiles Slang to WGSL for
VTK. It needs CMake 3.21 and a C++17 compiler, and downloads a Slang release when
it can't find one:

```sh
git clone https://github.com/sankhesh/SlangKit.git
cmake -S SlangKit -B SlangKit-build -DCMAKE_BUILD_TYPE=Release
cmake --build SlangKit-build
cmake --install SlangKit-build --prefix "$HOME/slangkit"
```

## Build VTK with Slang

Turn on the WebGPU backend and point VTK at SlangKit:

```sh
cmake -S vtk -B vtk-build -G Ninja \
  -DVTK_ENABLE_WEBGPU=ON \
  -DVTK_USE_SLANG=ON \
  -DSlangKit_DIR="$HOME/slangkit/lib/cmake/SlangKit"
cmake --build vtk-build
```

`VTK_USE_SLANG` needs `VTK_ENABLE_WEBGPU` and isn't available with Emscripten.
VTK links SlangKit itself, so an application only calls `find_package(VTK)`.

## Build and run the example

```sh
cmake -S vtk/Examples/Slang/Cxx/CustomShading -B example-build \
  -DVTK_DIR=vtk-build
cmake --build example-build
./example-build/CustomShading
```

If Dawn's library isn't on the library path, point `VTK_WEBGPU_LIBRARY` at it
(`libwebgpu_dawn.so`, `.dylib` or `.dll`). The example reads its modules from
this directory; pass another directory as the last argument to use your own
copies. `CustomShading --test` renders every mode offscreen and checks the
images instead of opening a window, which is what `ctest` runs.
