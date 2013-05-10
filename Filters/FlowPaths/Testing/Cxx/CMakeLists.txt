vtk_add_test_cxx(Graphics
  TestBSPTree.cxx
  TestStreamTracer.cxx
  TestAMRInterpolatedVelocityField.cxx
  TestParticleTracers.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)
