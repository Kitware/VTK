set(VTK_USE_EXTERNAL ON CACHE STRING "")

# These libraries are not supported right now.
set(VTK_MODULE_USE_EXTERNAL_VTK_libharu OFF CACHE BOOL "")
set(VTK_MODULE_USE_EXTERNAL_VTK_exprtk OFF CACHE BOOL "")
set(VTK_MODULE_USE_EXTERNAL_VTK_ioss OFF CACHE BOOL "")
set(VTK_MODULE_USE_EXTERNAL_VTK_fmt OFF CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora33.cmake")
