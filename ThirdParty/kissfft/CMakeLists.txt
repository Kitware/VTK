set(KISSFFT_SCALAR_TYPE "double")

vtk_module_third_party_internal(
  LICENSE_FILES           "vtkkissfft/COPYING"
  SPDX_LICENSE_IDENTIFIER "BSD-3-Clause"
  SPDX_COPYRIGHT_TEXT     "Copyright (c) 2003-2010 Mark Borgerding"
  SPDX_DOWNLOAD_LOCATION  "git+https://gitlab.kitware.com/third-party/kissfft.git@for/vtk-20211123-master-g6f09c27"
  VERSION                 "master"
  STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_kissfft.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_kissfft.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_kissfft.h")
