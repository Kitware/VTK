set(vtkkwiml_LIBRARIES "")

vtk_module_impl()

vtk_module_export_info()

configure_file(vtk_kwiml.h.in vtk_kwiml.h @ONLY)
if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vtk_kwiml.h
          DESTINATION ${VTK_INSTALL_INCLUDE_DIR}
          COMPONENT Development)
endif()

if(NOT VTK_INSTALL_NO_DEVELOPMENT)
  set(KWIML_INSTALL_INCLUDE_DIR ${VTK_INSTALL_INCLUDE_DIR})
  set(KWIML_INSTALL_INCLUDE_OPTIONS COMPONENT Development)
  set(KWIML_INCLUDE_PREFIX vtkkwiml)
endif()
if(BUILD_TESTING)
  set(KWIML_TEST_ENABLE 1)
  set(KWIML_TEST_PREFIX vtkkwiml)
endif()
add_subdirectory(vtkkwiml)
