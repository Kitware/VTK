vtk_add_test_cxx(vtksqliteCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestIncludeSQLite.cxx)
vtk_test_cxx_executable(vtksqliteCxxTests tests)
