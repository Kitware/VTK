vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo pugixml_repo
    tag  pugixml_tag
  VERSION_FROM_TAG
    tag  pugixml_version "^for/vtk-[0-9]*-master-([0-9\\.]+)-g[0-9a-f]+$")

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkpugixml/LICENSE.md"
    SPDX_LICENSE_IDENTIFIER
      "MIT"
    SPDX_COPYRIGHT_TEXT
      "Copyright (c) 2006-2020 Arseny Kapoulkine"
    SPDX_DOWNLOAD_LOCATION
      "git+${pugixml_repo}@${pugixml_tag}"
    VERSION
      "${pugixml_version}"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE pugixml
    TARGETS pugixml::pugixml
            pugixml
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_pugixml.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_pugixml.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_pugixml.h")
