vtk_add_test_cxx(vtkFiltersTemporalCxxTests tests
  TestForceStaticMesh.cxx
  )
vtk_test_cxx_executable(vtkFiltersTemporalCxxTests tests
  RENDERING_FACTORY
  )
