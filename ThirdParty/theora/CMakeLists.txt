set(theora_external_targets THEORA::THEORA)
if (NOT WIN32 OR (WIN32 AND (MSYS OR MINGW)))
  list(APPEND theora_external_targets
    THEORA::ENC
    THEORA::DEC)
endif ()

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtktheora/COPYING"
    SPDX_LICENSE_IDENTIFIER
      "BSD-3-Clause"
    SPDX_COPYRIGHT_TEXT
      "Copyright (C) 2002-2009 Xiph.org Foundation"
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/theora.git@for/vtk-20220517-1.1.1"
    VERSION
      "1.1.1"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE THEORA
    TARGETS ${theora_external_targets}
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_theora.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_theora.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_theora.h")
