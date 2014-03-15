vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestSampleFunction.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
