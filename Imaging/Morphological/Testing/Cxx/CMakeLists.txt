vtk_add_test_cxx(vtkImagingMorphologicalCxxTests tests
  TestImageThresholdConnectivity.cxx
  TestImageConnectivityFilter.cxx
  )

vtk_test_cxx_executable(vtkImagingMorphologicalCxxTests tests
  DISABLE_FLOATING_POINT_EXCEPTIONS
  RENDERING_FACTORY
  )
