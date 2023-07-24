vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkdoubleconversion/COPYING"
      "vtkdoubleconversion/LICENSE"
    SPDX_LICENSE_IDENTIFIER
      "BSD-3-Clause"
    SPDX_COPYRIGHT_TEXT
      "Copyright 2006-2011, the V8 project authors"
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/double-conversion.git@for/vtk-20191226-3.1.5"
    VERSION
      "3.1.5"
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
