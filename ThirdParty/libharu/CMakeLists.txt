vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtklibharu/LICENCE"
    VERSION       "2.4.0"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE LibHaru
    # Unreleased. Requires these PRs:
    # https://github.com/libharu/libharu/pull/157
    # https://github.com/libharu/libharu/pull/187
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
