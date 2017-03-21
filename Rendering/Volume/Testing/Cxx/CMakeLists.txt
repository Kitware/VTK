# These tests are built under all conditions
set (GenericVolumeCxxTests
  #  TestGPURayCastCompositeShadeMask.cxx
  ProjectedTetrahedraZoomIn.cxx,NO_VALID
  TestFinalColorWindowLevel.cxx
  TestFixedPointRayCastLightComponents.cxx
  TestGPURayCastAdditive.cxx
  TestGPURayCastCompositeBinaryMask.cxx
  TestGPURayCastCompositeMaskBlend.cxx
  TestGPURayCastCompositeMask.cxx
  TestGPURayCastCompositeToMIP.cxx
  TestGPURayCastCropping.cxx
  TestGPURayCastDataTypesMinIP.cxx
  TestGPURayCastDataTypesMIP.cxx
  TestGPURayCastFourComponentsComposite.cxx
  TestGPURayCastFourComponentsCompositeStreaming.cxx
  TestGPURayCastFourComponentsMinIP.cxx
  TestGPURayCastFourComponentsMIP.cxx
  TestGPURayCastMapperBenchmark.cxx
  TestGPURayCastMIPBinaryMask.cxx
  TestGPURayCastMIPToComposite.cxx
  TestGPURayCastNearestDataTypesMIP.cxx
  TestGPURayCastPerspectiveParallel.cxx
  TestGPURayCastVolumeOrientation.cxx
  TestGPURayCastVolumeUpdate.cxx
  TestGPUVolumeRayCastMapper.cxx
  TestMinIntensityRendering.cxx
  TestProjectedTetrahedra.cxx
  TestSmartVolumeMapper.cxx
  TestSmartVolumeMapperWindowLevel.cxx
  TestGPURayCastCompositeBinaryMask1.cxx
  )

# These tests are built for the OpenGL backend
set (VolumeOpenGLCxxTests
  HomogeneousRayIntegration.cxx
  LinearRayIntegration.cxx
  PartialPreIntegration.cxx
  PreIntegrationIncremental.cxx
  PreIntegrationNonIncremental.cxx
  TestGPURayCastCompositeShadeMask.cxx # This test depends on vtkOpenGLExtensionManager
  TestHAVSVolumeMapper.cxx
  TestProjectedHexahedra.cxx
  ZsweepConcavities.cxx
  )

if (NOT VTK_LEGACY_REMOVE)
  list (APPEND VolumeOpenGLCxxTests
    TestProp3DFollower.cxx
    TestTM3DLightComponents.cxx
    volProt.cxx)
endif()

# These tests are only built for the OpenGL2 backend
set (VolumeOpenGL2CxxTests
  TestGPURayCastAverageIP.cxx
  TestGPURayCastBlendModes.cxx
  TestGPURayCastCameraInside.cxx
  TestGPURayCastCameraInsideClipping.cxx
  TestGPURayCastCameraInsideSmallSpacing.cxx
  TestGPURayCastCameraInsideTransformation.cxx
  TestGPURayCastCellData.cxx
  TestGPURayCastClipping.cxx
  TestGPURayCastClippingPolyData.cxx
  TestGPURayCastClippingUserTransform.cxx
  TestGPURayCastDependentComponentsLightParameters.cxx
  TestGPURayCastDepthPeeling.cxx
  TestGPURayCastDepthPeelingBlendModes.cxx
  TestGPURayCastDepthPeelingOpaque.cxx
  TestGPURayCastDepthPeelingTransVol.cxx
  TestGPURayCastFourComponentsAdditive.cxx
  TestGPURayCastFourComponentsAverage.cxx
  TestGPURayCastFourComponentsDependentGradient.cxx
  TestGPURayCastGradientOpacity.cxx
  TestGPURayCastGradientOpacityLight.cxx
  TestGPURayCastImageSampleXY.cxx
  TestGPURayCastIndependentComponentsLightParameters.cxx
  TestGPURayCastIndependentVectorMode.cxx
  TestGPURayCastJittering.cxx
  TestGPURayCastJitteringCustom.cxx
  TestGPURayCastLargeColorTransferFunction.cxx
  TestGPURayCastMapperSampleDistance.cxx
  TestGPURayCastPositionalLights.cxx
  TestGPURayCastReleaseResources.cxx
  TestGPURayCastRenderDepthToImage.cxx
  TestGPURayCastRenderDepthToImage2.cxx
  TestGPURayCastRenderToTexture.cxx
  TestGPURayCastTextureStreaming.cxx
  TestGPURayCastThreeComponentsAdditive.cxx
  TestGPURayCastThreeComponentsIndependent.cxx
  TestGPURayCastTwoComponentsDependent.cxx
  TestGPURayCastTwoComponentsDependentGradient.cxx
  TestGPURayCastTwoComponentsGradient.cxx
  TestGPURayCastTwoComponentsIndependent.cxx
  TestGPURayCastVolumeDepthPass.cxx
  TestGPURayCastVolumeLightKit.cxx
  TestGPURayCastVolumePicking.cxx
  TestGPURayCastVolumePolyData.cxx
  TestGPURayCastVolumeRotation.cxx
  TestGPURayCastVolumeScale.cxx
  TestMultiBlockMapper.cxx
  TestProjectedTetrahedraOffscreen.cxx
  TestProjectedTetrahedraTransform.cxx
  TestRemoveVolumeNonCurrentContext.cxx
  )

# everyone gets these tests
vtk_add_test_cxx(${vtk-module}CxxTests tests
  ${GenericVolumeCxxTests}
  )

# If OpenGL2 backend, add all VolumeOpenGL2CxxTests
if ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  vtk_add_test_cxx(${vtk-module}CxxTests tests
    ${VolumeOpenGL2CxxTests}
    )
endif ()

if ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  vtk_test_cxx_executable(${vtk-module}CxxTests tests
    RENDERING_FACTORY
    )
  return()
endif ()

vtk_add_test_cxx(${vtk-module}CxxTests tests
  ${VolumeOpenGLCxxTests}
  )

vtk_test_cxx_executable(${vtk-module}CxxTests tests
  RENDERING_FACTORY
  ExerciseUnstructuredGridRayCastMapper.cxx)

if (NOT VTK_LEGACY_REMOVE)
  if(NOT VTK_TEST_TIMEOUT_TestProp3DFollower)
    set(VTK_TEST_TIMEOUT_TestProp3DFollower 100)
  endif()
  set_tests_properties(${vtk-module}Cxx-TestProp3DFollower
    PROPERTIES TIMEOUT ${VTK_TEST_TIMEOUT_TestProp3DFollower})
endif()

if (NOT VTK_LEGACY_REMOVE)
  # Allow local customization of timeout for this test.
  if(NOT VTK_TEST_TIMEOUT_TestTM3DLightComponents)
    set(VTK_TEST_TIMEOUT_TestTM3DLightComponents 120)
  endif()
  set_tests_properties(${vtk-module}Cxx-TestTM3DLightComponents
    PROPERTIES TIMEOUT ${VTK_TEST_TIMEOUT_TestTM3DLightComponents})
endif()
