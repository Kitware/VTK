set(tests
  TestIncludeKissFFT.cxx)

vtk_add_test_cxx(vtkkissfftCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtkkissfftCxxTests tests)
