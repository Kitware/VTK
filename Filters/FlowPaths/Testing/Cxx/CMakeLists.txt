vtk_add_test_cxx(vtkFiltersFlowPathsCxxTests tests
  TestBSPTree.cxx
  TestEvenlySpacedStreamlines2D.cxx
  TestStreamTracer.cxx,NO_VALID
  TestStreamTracerSurface.cxx
  TestStreamSurface.cxx
  TestAMRInterpolatedVelocityField.cxx,NO_VALID
  TestParallelVectors.cxx
  TestParticleTracers.cxx,NO_VALID
  TestLagrangianIntegrationModel.cxx,NO_VALID
  TestLagrangianParticle.cxx,NO_VALID
  TestLagrangianParticleTracker.cxx
  TestVortexCore.cxx,NO_VALID
  TestVectorFieldTopology.cxx
  )
vtk_test_cxx_executable(vtkFiltersFlowPathsCxxTests tests
  RENDERING_FACTORY
  )
