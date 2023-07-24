vtk_module_third_party_internal(
    LICENSE_FILES
      "vtkloguru/LICENSE"
    SPDX_LICENSE_IDENTIFIER
      "LicenseRef-PublicDomain-loguru"
    SPDX_COPYRIGHT_TEXT
      "NONE"
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/loguru.git@for/vtk-20230518-2.1.0"
    SPDX_CUSTOM_LICENSE_FILE
      "vtkloguru/LICENSE"
    SPDX_CUSTOM_LICENSE_NAME
      "PublicDomain-loguru"
    VERSION
      "2.1"
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_loguru.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_loguru.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_loguru.h")
