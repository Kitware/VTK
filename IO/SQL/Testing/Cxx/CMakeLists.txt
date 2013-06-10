vtk_add_test_cxx(NO_VALID
  TestSQLDatabaseSchema.cxx
  TestSQLiteDatabase.cxx
  )

set(TestSQLiteTableReadWrite_ARGS DATA{../Data/Input/simple_table.vtk})
vtk_add_test_cxx(TestSQLiteTableReadWrite.cxx NO_DATA NO_VALID NO_OUTPUT)
set_tests_properties(vtkIOSQLCxx-TestSQLiteTableReadWrite PROPERTIES RUN_SERIAL 1)

vtk_test_cxx_executable(${vtk-module}CxxTests)

# Run these serial to avoid intermittent test failures on machines
# that do parallel testing
set_tests_properties(vtkIOSQLCxx-TestSQLDatabaseSchema PROPERTIES RUN_SERIAL 1)
set_tests_properties(vtkIOSQLCxx-TestSQLiteDatabase PROPERTIES RUN_SERIAL 1)
