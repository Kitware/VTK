# NOTE: for now we are not mangling the externals, but in the future
# we could be using VTK_ABI_NAMESPACE_MANGLE macro to do all of these
# too.  In which case we would want to turn this OFF.
set(VTK_USE_EXTERNAL ON CACHE STRING "")

# These libraries are not supported right now.
set(VTK_MODULE_USE_EXTERNAL_VTK_exprtk OFF CACHE BOOL "")
set(VTK_MODULE_USE_EXTERNAL_VTK_ioss OFF CACHE BOOL "")
set(VTK_MODULE_USE_EXTERNAL_VTK_fast_float OFF CACHE BOOL "")
set(VTK_MODULE_USE_EXTERNAL_VTK_fmt OFF CACHE BOOL "")
set(VTK_MODULE_USE_EXTERNAL_VTK_libharu OFF CACHE BOOL "")
set(VTK_MODULE_USE_EXTERNAL_VTK_scn OFF CACHE BOOL "")
set(VTK_MODULE_USE_EXTERNAL_VTK_token OFF CACHE BOOL "")
set(VTK_MODULE_USE_EXTERNAL_VTK_verdict OFF CACHE BOOL "")
set(VTK_MODULE_USE_EXTERNAL_VTK_vtkviskores OFF CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora42.cmake")
