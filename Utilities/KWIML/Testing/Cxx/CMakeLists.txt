set(tests
  TestIncludeKWIML.cxx)

vtk_add_test_cxx(vtkkwimlCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtkkwimlCxxTests tests)
