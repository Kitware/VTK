# Currently Fides does not support VTK-m+CUDA
# issue: https://gitlab.kitware.com/vtk/fides/-/issues/13
set(VTK_MODULE_ENABLE_VTK_fides NO CACHE STRING "")

# Lowest-common denominator.
set(VTKm_CUDA_Architecture "pascal" CACHE STRING "")

# catalyst is not installed on cuda image
set(VTK_ENABLE_CATALYST OFF CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora34.cmake")
