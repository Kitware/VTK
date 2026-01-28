vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo loguru_repo
    tag  loguru_tag
  VERSION_FROM_TAG
    tag  loguru_version "^for/vtk-[0-9]*-([0-9\\.]+)$")

vtk_module_third_party_internal(
    LICENSE_FILES
      "vtkloguru/LICENSE"
    SPDX_LICENSE_IDENTIFIER
      "LicenseRef-PublicDomain-loguru"
    SPDX_COPYRIGHT_TEXT
      "NONE"
    SPDX_DOWNLOAD_LOCATION
      "git+${loguru_repo}@${loguru_tag}"
    SPDX_CUSTOM_LICENSE_FILE
      "vtkloguru/LICENSE"
    SPDX_CUSTOM_LICENSE_NAME
      "PublicDomain-loguru"
    VERSION
      "${loguru_version}"
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_loguru.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_loguru.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_loguru.h")
