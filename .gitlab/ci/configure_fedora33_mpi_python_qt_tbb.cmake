set(VTK_USE_EXTERNAL ON CACHE STRING "")

# These libraries are not supported right now.
set(VTK_MODULE_USE_EXTERNAL_VTK_libharu OFF CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora33.cmake")
