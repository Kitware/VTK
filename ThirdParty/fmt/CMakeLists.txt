vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtkfmt/LICENSE.rst"
    VERSION       "8.0.1"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE fmt
    TARGETS fmt::fmt
    # 8.1.0 is a version which is unreleased.
    # The required PR from 8.1.0 is https://github.com/fmtlib/fmt/pull/2432
    VERSION       "8.1.0"
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_fmt.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_fmt.h")


vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_fmt.h")
