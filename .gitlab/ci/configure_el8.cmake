# catalyst is not installed on el8 image
set(VTK_ENABLE_CATALYST OFF CACHE BOOL "")

# OpenXR is not installed on el8 image
set(VTK_MODULE_ENABLE_VTK_RenderingOpenXR NO CACHE STRING "")

# nasm is not installed on el8 image
set(VTK_JPEG_ENABLE_SIMD OFF CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora_common.cmake")
