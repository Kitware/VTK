#[==[
Preloads known platform-check results for a pinned Emscripten SDK.

A fresh VTK configure performs hundreds of serial try_compile calls. Each one
starts emcc's Python driver for both compilation and linking. These results
describe the Emscripten platform and are stable for a pinned SDK, so caching
them turns hundreds of try_compile calls into a handful of file(READ)s.

wasm32 and wasm64 are cached separately because their ABI sizes differ.

Used both by vtkEmscriptenToolchain.cmake (developer/local builds) and the
`.gitlab/ci/configure_wasm*.cmake` cache files (CI), so requires only:

  VTK_EMSCRIPTEN_PRELOAD_CACHE  Set to enable preloading at all.
  VTK_WEBASSEMBLY_64_BIT        Selects the wasm64 cache instead of wasm32.
  vtk_emsdk_version             From vtkToolVersions.cmake; overridden by
                                 VTK_EMSDK_VERSION, the same way the toolchain
                                 file resolves the SDK to install.
#]==]

if (NOT VTK_EMSCRIPTEN_PRELOAD_CACHE)
  return ()
endif ()
set(_vtk_emsdk_preload_version "${vtk_emsdk_version}")
if (VTK_EMSDK_VERSION)
  set(_vtk_emsdk_preload_version "${VTK_EMSDK_VERSION}")
endif ()

if (VTK_WEBASSEMBLY_64_BIT)
  set(_vtk_emsdk_preload_arch "wasm64")
else ()
  set(_vtk_emsdk_preload_arch "wasm32")
endif ()

set(_vtk_emsdk_preload_cache
  "${CMAKE_CURRENT_LIST_DIR}/vtkEmscriptenCache-${_vtk_emsdk_preload_version}-${_vtk_emsdk_preload_arch}.cmake")
include("${_vtk_emsdk_preload_cache}" OPTIONAL)

unset(_vtk_emsdk_preload_version)
unset(_vtk_emsdk_preload_arch)
unset(_vtk_emsdk_preload_cache)
