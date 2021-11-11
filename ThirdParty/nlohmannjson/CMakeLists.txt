vtk_module_third_party(
  INTERNAL
    INTERFACE
    LICENSE_FILES "vtknlohmannjson/LICENSE.MIT"
    VERSION       "3.10.4"
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
