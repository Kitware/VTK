if(VTK_ODBC_TEST_DSN)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkIOODBCTestingCxxConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkIOODBCTestingCxxConfigure.h)

  include_directories(${CMAKE_CURRENT_BINARY_DIR})

  vtk_add_test_cxx(TestODBCDatabase.cxx NO_DATA NO_VALID)
  vtk_test_cxx_executable(${vtk-module}CxxTests)
endif()
