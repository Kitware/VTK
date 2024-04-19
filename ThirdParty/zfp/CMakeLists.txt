vtk_module_third_party_internal(
  LICENSE_FILES
    "vtkzfp/LICENSE"
  SPDX_LICENSE_IDENTIFIER
    "BSD-3-Clause"
  SPDX_COPYRIGHT_TEXT
    "Copyright (c) 2014-2019, Lawrence Livermore National Security, LLC."
  SPDX_DOWNLOAD_LOCATION
    "git+https://gitlab.kitware.com/third-party/zfp.git@for/vtk-20190605-0.5.5"
  VERSION
    "0.5.5"
  STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_zfp.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_zfp.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_zfp.h")
