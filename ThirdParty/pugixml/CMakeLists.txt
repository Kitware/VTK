vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtkpugixml/LICENSE"
    VERSION       "1.10"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE pugixml
    TARGETS pugixml
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_pugixml.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_pugixml.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_pugixml.h")
