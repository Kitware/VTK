vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo utfcpp_repo
    tag  utfcpp_tag
  VERSION_FROM_TAG
    tag  utfcpp_version "^for/vtk-[0-9]*-([0-9\\.]+)$")

vtk_module_third_party(
  INTERNAL
    HEADER_ONLY
    LICENSE_FILES
      "vtkutf8/LICENSE"
    SPDX_LICENSE_IDENTIFIER
      "BSL-1.0"
    SPDX_COPYRIGHT_TEXT
      "Copyright 2006 Nemanja Trifunovic"
    SPDX_DOWNLOAD_LOCATION
      "git+${utfcpp_repo}@${utfcpp_tag}"
    VERSION       "${utfcpp_version}"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE utf8cpp
    TARGETS utf8cpp::utf8cpp
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_utf8.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_utf8.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_utf8.h")
