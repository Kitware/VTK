set(TestSQLiteTableReadWrite_ARGS DATA{../Data/Input/simple_table.vtk})
vtk_add_test_cxx(vtkIOSQLCxxTests tests
  NO_VALID
  TestSQLDatabaseSchema.cxx
  TestSQLiteDatabase.cxx
  TestSQLiteTableReadWrite.cxx,NO_DATA,NO_OUTPUT
  )
vtk_test_cxx_executable(vtkIOSQLCxxTests tests)

# Run these serial to avoid intermittent test failures on machines
# that do parallel testing
foreach(test IN LISTS tests)
  set_tests_properties(VTK::IOSQLCxx-${test}
    PROPERTIES
      RUN_SERIAL 1)
endforeach()
