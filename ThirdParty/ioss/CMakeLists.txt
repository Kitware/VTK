vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkioss/COPYRIGHT"
    SPDX_LICENSE_IDENTIFIER
      "BSD-3-Clause"
    SPDX_COPYRIGHT_TEXT
      "Copyright(C) 1999-2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS)"
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/seacas.git@ioss/for/vtk-20250313-v2025-02-27/seacas/libraries/ioss/"
    VERSION
      "2025-02-27"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE SEACASIoss
    TARGETS Ioss Ionit
    STANDARD_INCLUDE_DIRS)

if (VTK_MODULE_USE_EXTERNAL_vtkioss)
  # The usage requirements for Ioss' exported targets is not useful for us.
  # https://github.com/sandialabs/seacas/issues/239
  vtk_module_include(VTK::ioss
    INTERFACE
      "${SEACASIoss_INCLUDE_DIRS}")
endif ()

if (TARGET VTK::mpi)
  # The MPI optional dependency is not well-guarded in IOSS headers.
  vtk_module_link(VTK::ioss
    INTERFACE
      VTK::mpi)
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_ioss.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_ioss.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_ioss.h")
