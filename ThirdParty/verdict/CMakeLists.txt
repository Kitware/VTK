vtk_module_third_party_internal(
  LICENSE_FILES "../../Copyright.txt"
  VERSION       "1.2.0"
  STANDARD_INCLUDE_DIRS)

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_SOURCE_DIR}/vtk_verdict.h")
