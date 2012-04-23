if(VTK_MYSQL_TEST_URL)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkIOMySQLTestingCxxConfigure.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/vtkIOMySQLTestingCxxConfigure.h)

  include_directories(${CMAKE_CURRENT_BINARY_DIR})

  create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
    TestMySQLDatabase.cxx
    TestMySQLTableReadWrite.cxx
    EXTRA_INCLUDE vtkTestDriver.h
  )

  vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

  set(TestsToRun ${Tests})
  list(REMOVE_ITEM TestsToRun ${vtk-module}CxxTests.cxx)

  add_test(NAME ${vtk-module}Cxx-TestMySQLDatabase
           COMMAND ${vtk-module}CxxTests TestMySQLDatabase)

  if(VTK_DATA_ROOT)
    if(WIN32)
      add_test(NAME ${vtk-module}Cxx-TestMySQLTableReadWrite
               COMMAND ${vtk-module}CxxTests TestMySQLTableReadWrite
                       ${VTK_DATA_ROOT}/Baseline/IO/simple_table_win.vtk)
    else()
      add_test(NAME ${vtk-module}Cxx-TestMySQLTableReadWrite
               COMMAND ${vtk-module}CxxTests TestMySQLTableReadWrite
                       ${VTK_DATA_ROOT}/Baseline/IO/simple_table.vtk)
    endif()
  endif()

endif()
