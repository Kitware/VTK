create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
  TestSQLDatabaseSchema.cxx
  TestSQLiteDatabase.cxx
  TestSQLiteTableReadWrite.cxx
  EXTRA_INCLUDE vtkTestDriver.h
)

vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun ${vtk-module}CxxTests.cxx)
list(REMOVE_ITEM TestsToRun TestSQLiteTableReadWrite.cxx)

if(VTK_DATA_ROOT)
  add_test(NAME ${vtk-module}Cxx-SQLiteTableReadWrite
    COMMAND ${vtk-module}CxxTests TestSQLiteTableReadWrite
      ${VTK_DATA_ROOT}/Baseline/IO/simple_table.vtk
  )
  set_tests_properties(vtkIOSQLCxx-SQLiteTableReadWrite PROPERTIES RUN_SERIAL 1)
endif()

# Add all the executables
foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  if(VTK_DATA_ROOT)
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/IO/${TName}.png)
  else()
    add_test(NAME ${vtk-module}Cxx-${TName} COMMAND ${vtk-module}CxxTests ${TName})
  endif()

endforeach()

# Run these serial to avoid intermittent test failures on machines
# that do parallel testing
set_tests_properties(vtkIOSQLCxx-TestSQLDatabaseSchema PROPERTIES RUN_SERIAL 1)
set_tests_properties(vtkIOSQLCxx-TestSQLiteDatabase PROPERTIES RUN_SERIAL 1)
