vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestImageToPoints.cxx
  TestSampleFunction.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests
  RENDERING_FACTORY
  )
