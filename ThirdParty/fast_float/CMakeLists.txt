vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtkfast_float/LICENSE-MIT"
    VERSION "3.9.0"
    STANDARD_INCLUDE_DIRS
    HEADER_ONLY
  EXTERNAL 
    VERSION "3.9.0"
    PACKAGE FastFloat
    TARGETS FastFloat::fast_float
    )

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkfast_float.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkfast_float.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtkfast_float.h")
