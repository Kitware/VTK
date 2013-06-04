if(VTK_MYSQL_TEST_URL)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkIOMySQLTestingCxxConfigure.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/vtkIOMySQLTestingCxxConfigure.h)

  include_directories(${CMAKE_CURRENT_BINARY_DIR})

  vtk_add_test_cxx(TestMySQLDatabase.cxx NO_DATA NO_VALID)
  if(WIN32)
    set(TestMySQLTableReadWrite_ARGS
      DATA{../Data/Input/simple_table_win.vtk})
  else()
    set(TestMySQLTableReadWrite_ARGS
      DATA{../Data/Input/simple_table.vtk})
  endif()
  vtk_add_test_cxx(TestMySQLTableReadWrite.cxx NO_DATA NO_VALID)

  vtk_test_cxx_executable(${vtk-module}CxxTests)
endif()
