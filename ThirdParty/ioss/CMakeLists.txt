# FIXME: ioss never exports symbols properly
unset(CMAKE_CXX_VISIBILITY_PRESET)
unset(CMAKE_VISIBILITY_INLINES_HIDDEN)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

vtk_module_third_party_internal(
  LICENSE_FILES "vtkioss/COPYRIGHT"
  VERSION       "2020-04-27"
  STANDARD_INCLUDE_DIRS)

#configure_file(
#  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_ioss.h.in"
#  "${CMAKE_CURRENT_BINARY_DIR}/vtk_ioss.h")

vtk_module_install_headers(
#  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_ioss.h")
  FILES "${CMAKE_CURRENT_SOURCE_DIR}/vtk_ioss.h")
