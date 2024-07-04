vtk_add_test_cxx(vtkIOEnSightCxxTests tests
  TestEnSightReaderStaticMeshCache.cxx,NO_VALID
  )

vtk_test_cxx_executable(vtkIOEnSightCxxTests tests
  RENDERING_FACTORY
  )
