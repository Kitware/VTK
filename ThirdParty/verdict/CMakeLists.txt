vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkverdict/LICENSE"
    SPDX_LICENSE_IDENTIFIER
      "BSD-3-Clause"
    SPDX_COPYRIGHT_TEXT
      "Copyright 2003,2006,2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS)."
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/verdict.git@for/vtk-20220303-1.4.0"
    VERSION
      "1.4.0"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE Verdict
    TARGETS Verdict::verdict
    VERSION       "1.4.0"
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_verdict.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_verdict.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_verdict.h")
