vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtklibharu/LICENSE"
    SPDX_LICENSE_IDENTIFIER
      "Zlib"
    SPDX_COPYRIGHT_TEXT
      "Copyright (C) 1999-2006 Takeshi Kanno"
      "Copyright (C) 2007-2009 Antony Dovgal"
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/libharu.git@for/vtk-20220830-2.4.0"
    VERSION
      "2.4.0"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE LibHaru
    VERSION 2.4.0
    TARGETS LibHaru::LibHaru
    STANDARD_INCLUDE_DIRS)

include(vtkDetectLibraryType)
vtk_detect_library_shared(vtklibharu LibHaru::LibHaru)

set(HPDF_DLL 0)
if (WIN32 AND vtklibharu_is_shared)
  set(HPDF_DLL 1)
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_libharu.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_libharu.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_libharu.h")
