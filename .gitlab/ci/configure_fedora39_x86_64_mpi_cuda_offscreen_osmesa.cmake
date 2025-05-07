# Currently Fides does not support Viskores+CUDA
# issue: https://gitlab.kitware.com/vtk/fides/-/issues/13
set(VTK_MODULE_ENABLE_VTK_fides NO CACHE STRING "")

# Lowest-common denominator.
set(Viskores_CUDA_Architecture "pascal" CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora39_x86_64_cuda.cmake")
