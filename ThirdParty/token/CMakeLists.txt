# Override some configuration options before descending into vtktoken:
set(token_NAMESPACE "vtktoken")
set(token_PACKAGE_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake")

vtk_module_third_party(
  INTERNAL
    SUBDIRECTORY "vtktoken"
    LICENSE_FILES "vtktoken/license.md"
    SPDX_LICENSE_IDENTIFIER "BSD-3-Clause"
    SPDX_COPYRIGHT_TEXT "Copyright ⓒ  2020-2023 Kitware, Inc."
    SPDX_DOWNLOAD_LOCATION "git+https://gitlab.kitware.com/token/token.git@for/vtk"
    VERSION "23.09"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE token
    TARGETS token
    STANDARD_INCLUDE_DIRS
)
