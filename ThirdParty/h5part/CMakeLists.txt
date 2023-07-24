# we don't support external h5part
vtk_module_third_party_internal(
  LICENSE_FILES
    "vtkh5part/COPYING"
  SPDX_LICENSE_IDENTIFIER
    "BSD-3-Clause"
  SPDX_COPYRIGHT_TEXT
    "Copyright (c) 2006-2009, The Regents of the University of California, through Lawrence Berkeley National Laboratory (subject to receipt of any"
    "required approvals from the U.S. Dept. of Energy) and the Paul Scherrer Institut (Switzerland)."
  SPDX_DOWNLOAD_LOCATION
    "git+https://gitlab.kitware.com/third-party/h5part.git@for/vtk-20220119-1.6.6"
  VERSION
    "1.6.6"
  STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_h5part.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_h5part.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_h5part.h")
