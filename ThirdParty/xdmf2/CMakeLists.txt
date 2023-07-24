# FIXME: xdmf2 only exports symbols properly on Windows.
unset(CMAKE_CXX_VISIBILITY_PRESET)
unset(CMAKE_VISIBILITY_INLINES_HIDDEN)

vtk_module_third_party_internal(
  LICENSE_FILES
    "vtkxdmf2/Copyright.txt"
  SPDX_LICENSE_IDENTIFIER
    "LicenseRef-BSD-4-Clause-Modif"
  SPDX_COPYRIGHT_TEXT
    "Copyright (c) 2002 U.S. Army Research Laboratory"
  SPDX_DOWNLOAD_LOCATION
    "git+https://gitlab.kitware.com/xdmf/xdmf.git"
  SPDX_CUSTOM_LICENSE_FILE
    "vtkxdmf2/Copyright.txt"
  SPDX_CUSTOM_LICENSE_NAME
    "BSD-4-Clause-Modif"
  VERSION
    "1.2.11"
  STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_xdmf2.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_xdmf2.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_xdmf2.h")
