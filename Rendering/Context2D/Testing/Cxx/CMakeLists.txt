# Generic tests
vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestContext2D.cxx,NO_VALID
  TestPolyDataToContext.cxx
  )

vtk_test_cxx_executable(${vtk-module}CxxTests tests)
