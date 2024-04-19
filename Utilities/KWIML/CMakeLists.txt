if (BUILD_TESTING)
  set(KWIML_TEST_ENABLE 1)
  set(KWIML_TEST_PREFIX vtkkwiml)
endif ()

if (_vtk_build_INSTALL_HEADERS)
  set(KWIML_INSTALL_INCLUDE_DIR "${_vtk_build_HEADERS_DESTINATION}")
  set(KWIML_INCLUDE_PREFIX "vtkkwiml")
  set(KWIML_HEADERS_COMPONENT "${_vtk_build_HEADERS_COMPONENT}")
  if (_vtk_build_TARGET_SPECIFIC_COMPONENTS)
    string(PREPEND KWIML_HEADERS_COMPONENT "VTK::kwiml-")
  endif ()
  set(KWIML_INSTALL_INCLUDE_OPTIONS
    COMPONENT "${KWIML_HEADERS_COMPONENT}")
endif ()

vtk_module_third_party_internal(
  HEADER_ONLY
  LICENSE_FILES           "vtkkwiml/Copyright.txt"
  SPDX_LICENSE_IDENTIFIER "BSD-3-Clause"
  SPDX_COPYRIGHT_TEXT     "Copyright 2000-2016 Kitware, Inc. and Contributors"
  SPDX_DOWNLOAD_LOCATION  "git+https://gitlab.kitware.com/utils/kwiml@a079afc6"
  VERSION                 "1.0.0")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_SOURCE_DIR}/vtk_kwiml.h")
