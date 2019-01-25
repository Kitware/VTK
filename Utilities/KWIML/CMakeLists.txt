if (BUILD_TESTING)
  set(KWIML_TEST_ENABLE 1)
  set(KWIML_TEST_PREFIX vtkkwiml)
endif ()

if (_vtk_build_INSTALL_HEADERS)
  set(KWIML_INSTALL_INCLUDE_DIR "${_vtk_build_HEADERS_DESTINATION}")
  set(KWIML_INCLUDE_PREFIX "vtkkwiml")
  set(KWIML_INSTALL_INCLUDE_OPTIONS
    COMPONENT "${_vtk_build_HEADERS_COMPONENT}")
endif ()

vtk_module_third_party_internal(
  HEADER_ONLY
  LICENSE_FILES "vtkkwiml/Copyright.txt"
  VERSION       "1.0.0")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_SOURCE_DIR}/vtk_kwiml.h")

