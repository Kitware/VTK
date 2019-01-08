vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtkgl2ps/COPYING.GL2PS"
                  "vtkgl2ps/COPYING.LGPL"
    VERSION       "1.4.0"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE GL2PS
    # Unreleased. Requires these commits:
    # https://gitlab.onelab.info/gl2ps/gl2ps/commit/2b7018cce99dfdf3a338e796530551d89d5730ff
    # https://gitlab.onelab.info/gl2ps/gl2ps/commit/f3417eb98cff4bad0dbfbb7b5d3a0d69f8a10106
    VERSION 1.4.1
    TARGETS GL2PS::GL2PS
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_gl2ps.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_gl2ps.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_gl2ps.h")
