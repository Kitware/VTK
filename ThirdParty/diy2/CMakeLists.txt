vtk_module_third_party_internal(
  HEADER_ONLY
  LICENSE_FILES
    "vtkdiy2/LICENSE.txt"
    "vtkdiy2/LEGAL.txt"
  SPDX_LICENSE_IDENTIFIER
    "BSD-3-Clause"
  SPDX_COPYRIGHT_TEXT
    "Copyright (c) 2015, The Regents of the University of California, through Lawrence Berkeley National Laboratory (subject to receipt of any required approvals from the U.S. Dept. of Energy)"
  SPDX_DOWNLOAD_LOCATION
    "git+https://gitlab.kitware.com/third-party/diy2.git@for/vtk-20250313-e5f2edf6f"
  VERSION
    "3.6.0"
)

if (TARGET VTK::mpi)
  vtk_module_link(VTK::diy2
    INTERFACE
      VTK::mpi)

  vtk_module_definitions(VTK::diy2 INTERFACE DIY_HAS_MPI=1)
else ()
  vtk_module_definitions(VTK::diy2 INTERFACE DIY_HAS_MPI=0)
endif ()

# Link to the fmt library used by VTK
vtk_module_link(VTK::diy2
  INTERFACE
    VTK::fmt)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_diy2.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_diy2.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_diy2.h")
