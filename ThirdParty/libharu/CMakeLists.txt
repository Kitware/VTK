vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo libharu_repo
    tag  libharu_tag
  VERSION_FROM_TAG
    tag  libharu_version "^for/vtk-[0-9]*-([0-9\\.]+)-g[0-9a-f]+$")

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
      "git+${libharu_repo}@${libharu_tag}"
    VERSION
      "${libharu_version}"
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
