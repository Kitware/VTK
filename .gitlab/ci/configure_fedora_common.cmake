# Modules which require software not in the CI image.
set(VTK_MODULE_ENABLE_VTK_RenderingZSpace NO CACHE STRING "") # zSpace
set(VTK_MODULE_ENABLE_VTK_IOOCCT NO CACHE STRING "") # occt
set(VTK_MODULE_ENABLE_VTK_IOIFC NO CACHE STRING "") # IFC based on IfcOpenShell
set(VTK_MODULE_ENABLE_VTK_IOUSD NO CACHE STRING "") # usd
set(VTK_MODULE_ENABLE_VTK_IONanoVDB NO CACHE STRING "") # nanovdb
set(VTK_MODULE_ENABLE_VTK_IOOpenVDB NO CACHE STRING "") # openvdb
set(VTK_MODULE_ENABLE_VTK_conduit NO CACHE STRING "") # conduit

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "offscreen")
  set(VTK_USE_X OFF CACHE BOOL "")
else ()
  set(VTK_USE_X ON CACHE BOOL "")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "python" AND
    NOT "$ENV{CMAKE_CONFIGURATION}" MATCHES "offscreen")
  # only certain images have tcl/tk installed
  if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "fedora")
    # no tk until python 3.14 is available
    # set(VTK_USE_TK ON CACHE BOOL "")
  endif ()
endif ()

include("${CMAKE_CURRENT_LIST_DIR}/configure_common.cmake")
