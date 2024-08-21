vtk_add_test_cxx(vtkFiltersTemporalCxxTests tests
  TestForceStaticMesh.cxx, NO_VALID
  TestDataObjectMeshCache.cxx,NO_VALID
  TestTemporalSmoothingFilter.cxx,NO_VALID
  )

vtk_test_cxx_executable(vtkFiltersTemporalCxxTests tests
  RENDERING_FACTORY
  MeshCacheMockAlgorithms.cxx
  MeshCachePipeline.cxx
  )
