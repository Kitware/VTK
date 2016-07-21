vtk_add_test_cxx(${vtk-module}CxxTests tests
  UnitTestKernels.cxx,NO_VALID
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
