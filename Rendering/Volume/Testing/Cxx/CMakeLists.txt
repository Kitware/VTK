set (GPURayCastTests
  TestGPURayCastAdditive.cxx
  TestGPURayCastCompositeBinaryMask.cxx
  TestGPURayCastCompositeMask.cxx
  TestGPURayCastCompositeMaskBlend.cxx
  #  TestGPURayCastCompositeShadeMask.cxx
  TestGPURayCastCompositeToMIP.cxx
  TestGPURayCastCropping.cxx
  TestGPURayCastDataTypesMIP.cxx
  TestGPURayCastDataTypesMinIP.cxx
  TestGPURayCastFourComponentsComposite.cxx
  TestGPURayCastFourComponentsCompositeStreaming.cxx
  TestGPURayCastFourComponentsMIP.cxx
  TestGPURayCastFourComponentsMinIP.cxx
  TestGPURayCastMIPBinaryMask.cxx
  TestGPURayCastMIPToComposite.cxx
  TestGPURayCastNearestDataTypesMIP.cxx
  TestGPURayCastPerspectiveParallel.cxx
  )

set (VolumeOpenGLNew_GPURayCastTests
  #TestGPURayCastAdditive1.cxx
  TestGPURayCastClipping.cxx
  #TestGPURayCastCropping1.cxx
  #TestGPURayCastDataTypesMinIP1.cxx
  TestGPURayCastVolumePolyData.cxx
  TestGPURayCastVolumeRotation.cxx
  TestGPURayCastVolumeScale.cxx
  )

# everyone gets these tests
vtk_add_test_cxx(${vtk-module}CxxTests tests
  ${GPURayCastTests}
  TestProjectedTetrahedra.cxx
  ProjectedTetrahedraZoomIn.cxx,NO_VALID
  )

# If OpenGL2 backend, add all GPURayCastTests
if (("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2") OR
  (Module_vtkRenderingVolumeOpenGLNew))
  vtk_add_test_cxx(${vtk-module}CxxTests tests
    ${VolumeOpenGLNew_GPURayCastTests}
    )
endif ()

if ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  vtk_test_cxx_executable(${vtk-module}CxxTests tests
    RENDERING_FACTORY
    )
  return()
endif ()

vtk_add_test_cxx(${vtk-module}CxxTests tests
  #  TestGPUCastGradientOpacity.cxx
  HomogeneousRayIntegration.cxx
  LinearRayIntegration.cxx
  PartialPreIntegration.cxx
  PreIntegrationIncremental.cxx
  PreIntegrationNonIncremental.cxx
  TestFinalColorWindowLevel.cxx
  TestFixedPointRayCastLightComponents.cxx
  TestGPURayCastCompositeShadeMask.cxx # This test depends on vtkOpenGLExtensionManager
  TestHAVSVolumeMapper.cxx
  TestMinIntensityRendering.cxx
  TestProjectedHexahedra.cxx
  TestProp3DFollower.cxx
  TestSmartVolumeMapper.cxx
  TestSmartVolumeMapperWindowLevel.cxx
  TestTM3DLightComponents.cxx
  ZsweepConcavities.cxx
  volProt.cxx
  )

vtk_test_cxx_executable(${vtk-module}CxxTests tests
  RENDERING_FACTORY
  ExerciseUnstructuredGridRayCastMapper.cxx)

if(NOT VTK_TEST_TIMEOUT_TestProp3DFollower)
  set(VTK_TEST_TIMEOUT_TestProp3DFollower 100)
endif()
set_tests_properties(${vtk-module}Cxx-TestProp3DFollower
  PROPERTIES TIMEOUT ${VTK_TEST_TIMEOUT_TestProp3DFollower})

# Allow local customization of timeout for this test.
if(NOT VTK_TEST_TIMEOUT_TestTM3DLightComponents)
  set(VTK_TEST_TIMEOUT_TestTM3DLightComponents 120)
endif()
set_tests_properties(${vtk-module}Cxx-TestTM3DLightComponents
  PROPERTIES TIMEOUT ${VTK_TEST_TIMEOUT_TestTM3DLightComponents})
