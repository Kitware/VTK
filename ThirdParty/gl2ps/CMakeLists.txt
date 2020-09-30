vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtkgl2ps/COPYING.GL2PS"
                  "vtkgl2ps/COPYING.LGPL"
    VERSION       "1.4.2"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE GL2PS
    VERSION 1.4.2
    TARGETS GL2PS::GL2PS
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_gl2ps.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_gl2ps.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_gl2ps.h")
