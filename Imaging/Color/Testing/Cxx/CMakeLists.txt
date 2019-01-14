vtk_add_test_cxx(vtkImagingColorCxxTests tests
  ImageQuantizeToIndex.cxx,NO_VALID
  )

vtk_test_cxx_executable(vtkImagingColorCxxTests tests)
