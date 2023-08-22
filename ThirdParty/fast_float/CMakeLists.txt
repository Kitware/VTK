vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkfast_float/LICENSE-MIT"
    SPDX_LICENSE_IDENTIFIER
      "MIT"
    SPDX_COPYRIGHT_TEXT
      "Copyright (c) 2021 The fast_float authors"
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/fast_float.git@for/vtk-20230309-3.9.0"
    VERSION
      "3.9.0"
    STANDARD_INCLUDE_DIRS
    HEADER_ONLY
  EXTERNAL
    VERSION "3.9.0"
    PACKAGE FastFloat
    TARGETS FastFloat::fast_float
    STANDARD_INCLUDE_DIRS
    )

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkfast_float.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkfast_float.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtkfast_float.h")
