vtk_module_third_party_internal(
  LICENSE_FILES
    "vtkexodusII/COPYRIGHT"
  SPDX_LICENSE_IDENTIFIER
    "BSD-3-Clause"
  SPDX_COPYRIGHT_TEXT
    "Copyright (c) 2005-2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS)"
  SPDX_DOWNLOAD_LOCATION
    "git+https://gitlab.kitware.com/third-party/seacas.git@exodus/for/vtk-20240628-master-gaeee5aa1#packages/seacas/libraries/exodus"
  VERSION
    "9.3f"
  STANDARD_INCLUDE_DIRS)

#configure_file(
#  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_exodusII.h.in"
#  "${CMAKE_CURRENT_BINARY_DIR}/vtk_exodusII.h")

vtk_module_install_headers(
#  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_exodusII.h")
  FILES "${CMAKE_CURRENT_SOURCE_DIR}/vtk_exodusII.h")
