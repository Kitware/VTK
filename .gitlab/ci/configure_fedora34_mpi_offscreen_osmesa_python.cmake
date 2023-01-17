# Qt doesn't support OSMesa.
set(VTK_GROUP_ENABLE_Qt NO CACHE STRING "")

# OpenXR do not support OSMesa
set(VTK_MODULE_ENABLE_VTK_RenderingOpenXR NO CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora34.cmake")
