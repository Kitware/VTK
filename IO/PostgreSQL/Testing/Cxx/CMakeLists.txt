if(VTK_PSQL_TEST_URL)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkIOPostgresSQLTestingCxxConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkIOPostgresSQLTestingCxxConfigure.h)

  include_directories(${CMAKE_CURRENT_BINARY_DIR})

  vtk_add_test_cxx(TestPostgreSQLDatabase.cxx NO_DATA NO_VALID)
  if(WIN32)
    set(TestPostgreSQLTableReadWrite_ARGS
      DATA{../Data/Input/simple_table_win.vtk})
  else()
    set(TestPostgreSQLTableReadWrite_ARGS
      DATA{../Data/Input/simple_table.vtk})
  endif()
  vtk_add_test_cxx(TestPostgreSQLTableReadWrite.cxx NO_DATA NO_VALID)

  vtk_test_cxx_executable(${vtk-module}CxxTests)
endif()
