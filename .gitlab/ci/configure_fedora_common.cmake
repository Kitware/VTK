set(PARAVIEW_ENABLE_ADIOS ON CACHE BOOL "")

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "offscreen")
  set(VTK_USE_X OFF CACHE BOOL "")
else ()
  set(VTK_USE_X ON CACHE BOOL "")
endif ()

include("${CMAKE_CURRENT_LIST_DIR}/configure_common.cmake")
