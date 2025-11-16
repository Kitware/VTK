# Greatest-common value: Turing = 75
set(CMAKE_CUDA_ARCHITECTURES 75 CACHE STRING "")

# OpenXR do not support OSMesa
set(VTK_MODULE_ENABLE_VTK_RenderingOpenXR NO CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora39_x86_64_cuda.cmake")
