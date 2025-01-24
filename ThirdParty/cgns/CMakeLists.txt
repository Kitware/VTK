# note: the CGNS version is determined by contracting the minor and bugfix version
#       into a single version; 4.2.0 becomes 4.20. See src/cgnslib.h CGNS_DOTVERS constant.
# Furthermore, 4.1.x versions have been released with version 4.10, even when x > 0.
#
# For more information see:
# https://gitlab.kitware.com/vtk/vtk/-/issues/18654
# https://github.com/CGNS/CGNS/discussions/357

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkcgns/license.txt"
    SPDX_LICENSE_IDENTIFIER
      "Zlib"
    SPDX_COPYRIGHT_TEXT
      "© Copyright 1991-2020, CGNS Steering Commmittee"
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/cgns.git@for/vtk-20250124-4.5.0-2"
    VERSION
      "4.50"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE CGNS
    VERSION       "4.10"
    TARGETS CGNS::CGNS
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_cgns.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_cgns.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_cgns.h")
