vtk_module_third_party(
  INTERNAL
    HEADER_ONLY
    LICENSE_FILES 
      "vtkutf8/LICENSE"
    SPDX_LICENSE_IDENTIFIER
      "BSL-1.0"
    SPDX_COPYRIGHT_TEXT
      "Copyright 2006 Nemanja Trifunovic"
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/utfcpp.git@for/vtk-20181015-2.3.4"
    VERSION       "2.3.4"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE utf8cpp
    TARGETS utf8cpp::utf8cpp
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_utf8.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_utf8.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_utf8.h")
