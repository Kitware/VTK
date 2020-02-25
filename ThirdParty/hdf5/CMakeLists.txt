cmake_policy(PUSH)
cmake_policy(SET CMP0074 NEW)

set(HDF5_EXTERNALLY_CONFIGURED 1)

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtkhdf5/COPYING"
                  "vtkhdf5/COPYING_LBNL_HDF5"
    VERSION       "1.10.6"
    STANDARD_INCLUDE_DIRS
    INTERFACE
  EXTERNAL
    PACKAGE       HDF5
    COMPONENTS    C HL
    TARGETS       hdf5::hdf5
                  hdf5::hdf5_hl
    USE_VARIABLES HDF5_IS_PARALLEL
    STANDARD_INCLUDE_DIRS)

set(vtkhdf5_is_parallel 0)
if (VTK_MODULE_USE_EXTERNAL_vtkhdf5 AND HDF5_IS_PARALLEL)
  if (NOT TARGET VTK::mpi)
    message(FATAL_ERROR
      "An external MPI-aware HDF5 requires that VTK be built with MPI support "
      "as well.")
  endif ()

  set(vtkhdf5_is_parallel 1)
  vtk_module_link(VTK::hdf5
    INTERFACE
      VTK::mpi)
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_hdf5.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_hdf5.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_hdf5.h")

cmake_policy(POP)
