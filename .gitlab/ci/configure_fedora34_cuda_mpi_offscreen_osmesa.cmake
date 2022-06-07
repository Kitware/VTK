# Lowest-common denominator. Pascal = 60
set(CMAKE_CUDA_ARCHITECTURES 60 CACHE STRING "")

# catalyst is not installed on cuda image
set(VTK_ENABLE_CATALYST OFF CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora34.cmake")
