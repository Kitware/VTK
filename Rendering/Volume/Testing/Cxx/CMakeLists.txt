vtk_add_test_cxx(
  HomogeneousRayIntegration.cxx
  LinearRayIntegration.cxx
  PartialPreIntegration.cxx
  PreIntegrationIncremental.cxx
  PreIntegrationNonIncremental.cxx
  TestFixedPointRayCastLightComponents.cxx
  TestGPURayCastAdditive.cxx
  TestGPURayCastCompositeBinaryMask.cxx
  TestGPURayCastCompositeMask.cxx
  TestGPURayCastCompositeMaskBlend.cxx
  TestGPURayCastCompositeShadeMask.cxx
  TestGPURayCastCompositeToMIP.cxx
  TestGPURayCastCropping.cxx
  TestGPURayCastDataTypesMIP.cxx
  TestGPURayCastFourComponentsComposite.cxx
  TestGPURayCastFourComponentsCompositeStreaming.cxx
  TestGPURayCastFourComponentsMIP.cxx
  TestGPURayCastFourComponentsMinIP.cxx
  TestGPURayCastMIPBinaryMask.cxx
  TestGPURayCastMIPToComposite.cxx
  TestGPURayCastPerspectiveParallel.cxx
  TestProjectedHexahedra.cxx
  TestSmartVolumeMapper.cxx
  TestSmartVolumeMapperWindowLevel.cxx
  TestTM3DLightComponents.cxx
  ZsweepConcavities.cxx
  )

# Tests with data
vtk_add_test_cxx(
  ProjectedTetrahedraZoomIn.cxx,NO_VALID
  TestFinalColorWindowLevel.cxx
  TestGPURayCastNearestDataTypesMIP.cxx
  TestGPURayCastDataTypesMinIP.cxx
  TestHAVSVolumeMapper.cxx
  TestMinIntensityRendering.cxx
  TestProjectedTetrahedra.cxx
  TestProp3DFollower.cxx
  volProt.cxx
  )

vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY
  ExerciseUnstructuredGridRayCastMapper.cxx)

set_tests_properties(${vtk-module}Cxx-TestProp3DFollower
  PROPERTIES TIMEOUT 100)

# Allow local customization of timeout for this test.
if(NOT VTK_TEST_TIMEOUT_TestTM3DLightComponents)
  set(VTK_TEST_TIMEOUT_TestTM3DLightComponents 120)
endif()
set_tests_properties(${vtk-module}Cxx-TestTM3DLightComponents
  PROPERTIES TIMEOUT ${VTK_TEST_TIMEOUT_TestTM3DLightComponents})
