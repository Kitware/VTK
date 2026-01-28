vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo nlohmannjson_repo
    tag  nlohmannjson_tag
  VERSION_FROM_TAG
    tag  nlohmannjson_version "^for/vtk-[0-9]*-([0-9\\.]+)$")

vtk_module_third_party(
  INTERNAL
    INTERFACE
    LICENSE_FILES
      "vtknlohmannjson/LICENSE.MIT"
    SPDX_LICENSE_IDENTIFIER
      "MIT"
    SPDX_COPYRIGHT_TEXT
      "Copyright (c) 2013-2021 Niels Lohmann"
    SPDX_DOWNLOAD_LOCATION
      "git+${nlohmannjson_repo}@${nlohmannjson_tag}"
    VERSION
      "${nlohmannjson_version}"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE nlohmann_json
    TARGETS nlohmann_json::nlohmann_json
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_nlohmannjson.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_nlohmannjson.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_nlohmannjson.h")
