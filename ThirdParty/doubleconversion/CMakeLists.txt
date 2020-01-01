vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtkdoubleconversion/COPYING"
                  "vtkdoubleconversion/LICENSE"
    VERSION       "3.1.5"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE double-conversion
    TARGETS double-conversion::double-conversion
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_doubleconversion.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_doubleconversion.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_doubleconversion.h")
