# Lowest-common denominator. Pascal = 60
set(CMAKE_CUDA_ARCHITECTURES 60 CACHE STRING "")

# OpenXR do not support OSMesa
set(VTK_MODULE_ENABLE_VTK_RenderingOpenXR NO CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora39_cuda.cmake")
