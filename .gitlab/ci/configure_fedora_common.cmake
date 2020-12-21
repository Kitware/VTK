# Modules which require software not in the CI image.
set(VTK_MODULE_ENABLE_VTK_FiltersOpenTURNS NO CACHE STRING "") # openturns
set(VTK_MODULE_ENABLE_VTK_IOADIOS2 NO CACHE STRING "") # adios
set(VTK_MODULE_ENABLE_VTK_RenderingOpenVR NO CACHE STRING "") # openvr
set(VTK_MODULE_ENABLE_VTK_RenderingRayTracing NO CACHE STRING "") # ospray
set(VTK_MODULE_ENABLE_VTK_fides NO CACHE STRING "") # adois

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "offscreen")
  set(VTK_USE_X OFF CACHE BOOL "")
else ()
  set(VTK_USE_X ON CACHE BOOL "")
endif ()

include("${CMAKE_CURRENT_LIST_DIR}/configure_common.cmake")
