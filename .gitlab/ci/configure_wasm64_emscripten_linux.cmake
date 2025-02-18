# Ensure threads are not enabled.
set(VTK_WEBASSEMBLY_THREADS OFF CACHE BOOL "")
# Enable exceptions
set(VTK_WEBASSEMBLY_EXCEPTIONS ON CACHE BOOL "")
set(VTK_WEBASSEMBLY_64_BIT ON CACHE BOOL "")
# Temporarily disabled, see https://gitlab.kitware.com/vtk/vtk/-/issues/19617
set(VTK_ENABLE_WRAPPING OFF CACHE BOOL "")
# Temporarily disabled, see https://gitlab.kitware.com/vtk/vtk/-/issues/19617
set(VTK_WRAP_SERIALIZATION OFF CACHE BOOL "")
include("${CMAKE_CURRENT_LIST_DIR}/configure_wasm_common.cmake")
