# NOTE: for now we are not mangling the externals, but in the future
# we could be using VTK_ABI_NAMESPACE_MANGLE macro to do all of these
# too.  In which case we would want to turn this OFF.
set(VTK_USE_EXTERNAL ON CACHE STRING "")

# This tells VTK to use its internal vendored ThirdParty/ libraries in the build
# rather than what may or may not be available on the system.
set(VTK_MODULE_USE_EXTERNAL_VTK_libharu OFF CACHE BOOL "")
set(VTK_MODULE_USE_EXTERNAL_VTK_exprtk OFF CACHE BOOL "")
set(VTK_MODULE_USE_EXTERNAL_VTK_ioss OFF CACHE BOOL "")
set(VTK_MODULE_USE_EXTERNAL_VTK_fmt OFF CACHE BOOL "")
set(VTK_MODULE_USE_EXTERNAL_VTK_fast_float OFF CACHE BOOL "")
set(VTK_MODULE_USE_EXTERNAL_VTK_verdict OFF CACHE BOOL "")
set(VTK_MODULE_USE_EXTERNAL_VTK_token OFF CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora39.cmake")
