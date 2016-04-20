vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestBSPTree.cxx
  TestStreamTracer.cxx,NO_VALID
  TestStreamTracerSurface.cxx
  TestAMRInterpolatedVelocityField.cxx,NO_VALID
  TestParticleTracers.cxx,NO_VALID
  TestLagrangianIntegrationModel.cxx,NO_VALID
  TestLagrangianParticle.cxx,NO_VALID
  TestLagrangianParticleTracker.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests
  RENDERING_FACTORY
  )
