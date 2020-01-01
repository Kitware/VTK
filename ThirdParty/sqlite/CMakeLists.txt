vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "public-domain"
    VERSION       "3.30.1"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE SQLite3
    TARGETS SQLite::SQLite3
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_sqlite.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_sqlite.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_sqlite.h")
