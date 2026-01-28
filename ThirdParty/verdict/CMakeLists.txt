vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo verdict_repo
    tag  verdict_tag
  VERSION_FROM_TAG
    tag  verdict_version "^for/vtk-[0-9]*-([0-9\\.]+)$")

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkverdict/LICENSE"
    SPDX_LICENSE_IDENTIFIER
      "BSD-3-Clause"
    SPDX_COPYRIGHT_TEXT
      "Copyright 2003,2006,2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS)."
    SPDX_DOWNLOAD_LOCATION
      "git+${verdict_repo}@${verdict_tag}"
    VERSION
      "${verdict_version}"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE Verdict
    TARGETS Verdict::verdict
    VERSION       "1.4.4"
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_verdict.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_verdict.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_verdict.h")
