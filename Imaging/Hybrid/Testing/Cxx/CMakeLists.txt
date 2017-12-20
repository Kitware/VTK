vtk_add_test_cxx(vtkImagingHybridCxxTests tests
  TestImageToPoints.cxx
  TestSampleFunction.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  )
vtk_test_cxx_executable(vtkImagingHybridCxxTests tests
  RENDERING_FACTORY
  )
