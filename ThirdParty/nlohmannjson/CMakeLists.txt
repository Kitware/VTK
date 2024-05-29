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
      "git+https://gitlab.kitware.com/third-party/nlohmann_json.git@for/vtk-20240529-3.11.3"
    VERSION
      "3.11.3"
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
