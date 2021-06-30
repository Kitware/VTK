# Modules which require software not in the CI image.
set(VTK_MODULE_ENABLE_VTK_RenderingRayTracing NO CACHE STRING "") # ospray

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "offscreen")
  set(VTK_USE_X OFF CACHE BOOL "")
else ()
  set(VTK_USE_X ON CACHE BOOL "")
endif ()

include("${CMAKE_CURRENT_LIST_DIR}/configure_common.cmake")
