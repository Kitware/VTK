vtk_add_test_cxx(vtkFiltersFlowPathsCxxTests tests
  TestBSPTree.cxx
  TestCellLocatorsLinearTransform.cxx,NO_DATA,NO_VALID,NO_OUTPUT
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
  TestVectorFieldTopologyNoIterativeSeeding.cxx
  )

set(test_implicit_array)
if(TARGET VTK::CommonImplicitArrays)
  list(APPEND test_implicit_array
    TestStreamTracerImplicitArray.cxx
  )
endif()

vtk_add_test_cxx(vtkFiltersFlowPathsCxxTests imp_tests
  NO_VALID
  ${test_implicit_array}
)

set(all_tests
  ${tests}
  ${imp_tests}
)

vtk_test_cxx_executable(vtkFiltersFlowPathsCxxTests all_tests
  DISABLE_FLOATING_POINT_EXCEPTIONS
  RENDERING_FACTORY
  )
