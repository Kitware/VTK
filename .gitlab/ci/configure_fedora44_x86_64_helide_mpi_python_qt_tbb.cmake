set(VTK_USE_EXTERNAL ON CACHE STRING "")

# These libraries are not supported right now.
set(VTK_MODULE_USE_EXTERNAL_VTK_exprtk OFF CACHE BOOL "")
set(VTK_MODULE_USE_EXTERNAL_VTK_fides OFF CACHE BOOL "")
set(VTK_MODULE_USE_EXTERNAL_VTK_ioss OFF CACHE BOOL "")
set(VTK_MODULE_USE_EXTERNAL_VTK_scn OFF CACHE BOOL "")
set(VTK_MODULE_USE_EXTERNAL_VTK_token OFF CACHE BOOL "")
set(VTK_MODULE_USE_EXTERNAL_VTK_verdict OFF CACHE BOOL "")
set(VTK_MODULE_USE_EXTERNAL_VTK_vtkviskores OFF CACHE BOOL "")

# Enable Catalyst and Conduit to test their external integration
set(VTK_ENABLE_CATALYST ON CACHE BOOL "")
set(VTK_MODULE_ENABLE_VTK_conduit YES CACHE STRING "")
set(VTK_MODULE_ENABLE_VTK_fides YES CACHE STRING "")

set(CMAKE_PREFIX_PATH "/opt/catalyst-ext/mpich;/opt/conduit/mpich;/opt/anari" CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora44.cmake")
