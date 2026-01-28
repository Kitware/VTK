vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo fast_float_repo
    tag  fast_float_tag
  VERSION_FROM_TAG
    tag  fast_float_version "^for/vtk-[0-9]*-([0-9\\.]+)$")

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkfast_float/LICENSE-MIT"
    SPDX_LICENSE_IDENTIFIER
      "MIT"
    SPDX_COPYRIGHT_TEXT
      "Copyright (c) 2021 The fast_float authors"
    SPDX_DOWNLOAD_LOCATION
      "git+${fast_float_repo}@${fast_float_tag}"
    VERSION
      "${fast_float_version}"
    STANDARD_INCLUDE_DIRS
    HEADER_ONLY
  EXTERNAL
    VERSION "8.0.0"
    PACKAGE FastFloat
    TARGETS FastFloat::fast_float
    STANDARD_INCLUDE_DIRS
    )

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkfast_float.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkfast_float.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtkfast_float.h")
