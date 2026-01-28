set(HDF5_EXTERNALLY_CONFIGURED 1)

vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo hdf5_repo
    tag  hdf5_tag
  VERSION_FROM_TAG
    tag  hdf5_version "^for/vtk-[0-9]*-([0-9\\.]+)$")

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkhdf5/COPYING"
      "vtkhdf5/COPYING_LBNL_HDF5"
      "vtkhdf5/src/H5FDsubfiling/mercury/LICENSE.txt"
    SPDX_LICENSE_IDENTIFIER
      "BSD-3-Clause"
    SPDX_COPYRIGHT_TEXT
      "Copyright 2006 by The HDF Group."
      "Copyright 1998-2006 by The Board of Trustees of the University of Illinois."
      "Copyright (c) 2016, The Regents of the University of California, through Lawrence Berkeley National Laboratory (subject to receipt of any required approvals from the U.S. Dept. of Energy)."
      "Copyright (c) 2013-2022, UChicago Argonne, LLC and The HDF Group."
      "Copyright (c) 2022-2023, Intel Corporation."
    SPDX_DOWNLOAD_LOCATION
      "git+${hdf5_repo}@${hdf5_tag}"
    VERSION
      "${hdf5_version}"
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

cmake_dependent_option(
  VTK_HDF5_ENABLE_THREADSAFE
  "Enable thread-safe HDF5"
  OFF
  "NOT VTK_MODULE_USE_EXTERNAL_vtkhdf5"
  OFF)
mark_as_advanced(VTK_HDF5_ENABLE_THREADSAFE)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_hdf5.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_hdf5.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_hdf5.h")
