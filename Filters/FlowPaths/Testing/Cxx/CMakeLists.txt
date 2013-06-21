vtk_add_test_cxx(
  TestBSPTree.cxx
  TestStreamTracer.cxx,NO_VALID
  TestAMRInterpolatedVelocityField.cxx,NO_VALID
  TestParticleTracers.cxx,NO_VALID
  )
vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)
