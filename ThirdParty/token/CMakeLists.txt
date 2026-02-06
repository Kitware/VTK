# Override some configuration options before descending into vtktoken:
set(token_NAMESPACE "vtktoken")
set(token_PACKAGE_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake")
set(token_ENABLE_TESTING OFF)
set(token_FORCE_OLD_DTAGS OFF)

vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo token_repo)

vtk_module_third_party(
  INTERNAL
    SUBDIRECTORY "vtktoken"
    LICENSE_FILES "vtktoken/license.md"
    SPDX_LICENSE_IDENTIFIER "BSD-3-Clause"
    SPDX_COPYRIGHT_TEXT "Copyright ⓒ  2020-2023 Kitware, Inc."
    SPDX_DOWNLOAD_LOCATION "git+${token_repo}@for/vtk"
    VERSION "for/vtk"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE token
    TARGETS token
    STANDARD_INCLUDE_DIRS
)

unset(token_FORCE_OLD_DTAGS CACHE)
