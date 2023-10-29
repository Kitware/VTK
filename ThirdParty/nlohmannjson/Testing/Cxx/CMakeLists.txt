set(tests
  TestIncludeNlohmannJson.cxx)

vtk_add_test_cxx(vtknlohmannjsonCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtknlohmannjsonCxxTests tests)
