vtk_add_test_cxx(vtkFiltersFlowPathsCxxTests tests
  TestBSPTree.cxx
  TestEvenlySpacedStreamlines2D.cxx
  TestStreamTracer.cxx,NO_VALID
  TestStreamTracerSurface.cxx
  TestAMRInterpolatedVelocityField.cxx,NO_VALID
  TestParticleTracers.cxx,NO_VALID
  TestLagrangianIntegrationModel.cxx,NO_VALID
  TestLagrangianParticle.cxx,NO_VALID
  TestLagrangianParticleTracker.cxx
  )
vtk_test_cxx_executable(vtkFiltersFlowPathsCxxTests tests
  RENDERING_FACTORY
  )
