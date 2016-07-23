vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestTransform.cxx
  TestLandmarkTransform.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
