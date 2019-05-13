set(KISSFFT_SCALAR_TYPE "double")

vtk_module_third_party_internal(
  LICENSE_FILES "vtkkissfft/COPYING"
  VERSION       "master"
  STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_kissfft.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_kissfft.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_kissfft.h")
