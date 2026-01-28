vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo cli11_repo
    tag  cli11_tag
  VERSION_FROM_TAG
    tag  cli11_version "^for/vtk-[0-9]*-([0-9\\.]+)$")

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkcli11/LICENSE"
    SPDX_LICENSE_IDENTIFIER
      "BSD-3-Clause"
    SPDX_COPYRIGHT_TEXT
      "Copyright (c) 2017-2019 University of Cincinnati, developed by Henry Schreiner under NSF AWARD 1414736"
    SPDX_DOWNLOAD_LOCATION
      "git+${cli11_repo}@${cli11_tag}"
    VERSION
      "${cli11_version}"
    HEADER_ONLY
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE CLI11
    TARGETS CLI11::CLI11
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_cli11_external.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_cli11_external.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_SOURCE_DIR}/vtk_cli11.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/vtk_cli11_forward.h"
        "${CMAKE_CURRENT_BINARY_DIR}/vtk_cli11_external.h")
