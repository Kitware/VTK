vtk_module_third_party(
  INTERNAL
    LICENSE_FILES 
      "vtkpugixml/LICENSE.md"
    SPDX_LICENSE_IDENTIFIER
      "MIT"
    SPDX_COPYRIGHT_TEXT
      "Copyright (c) 2006-2020 Arseny Kapoulkine"
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/pugixml.git@for/vtk-20210727-1.11.4"
    VERSION       
      "1.11.4"
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
