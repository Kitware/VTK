vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      vtkscn/LICENSE
      vtkscn/LICENSE.nanorange
    SPDX_LICENSE_IDENTIFIER "Apache-2.0 AND BSL-1.0"
    SPDX_COPYRIGHT_TEXT     "Copyright (c) 2017 Elias Kosunen; Copyright (c) 2018 Tristan Brindle (tcbrindle at gmail dot com)"
    SPDX_DOWNLOAD_LOCATION  "git+https://gitlab.kitware.com/third-party/scnlib.git@for/vtk-20250416-4.0.1"
    VERSION                 "4.0.1"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE scn
    TARGETS scn::scn
    VERSION       "4.0.0"
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_scn.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_scn.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_scn.h")
