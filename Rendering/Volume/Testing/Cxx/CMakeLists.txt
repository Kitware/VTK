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
  TestGPURayCastMapperSampleDistance.cxx
  TestGPURayCastMIPBinaryMask.cxx
  TestGPURayCastMIPToComposite.cxx
  TestGPURayCastNearestDataTypesMIP.cxx
  TestGPURayCastPerspectiveParallel.cxx
  TestGPURayCastVolumeUpdate.cxx
  TestMinIntensityRendering.cxx
  TestProjectedTetrahedra.cxx
  TestSmartVolumeMapper.cxx
  TestSmartVolumeMapperWindowLevel.cxx
  )

# These tests are built only if either Module_vtkRenderingVolumeOpenGLNew is ON
# or if the rendering backend is OpenGL2
set (OpenGLNew_OR_OpenGL2_CxxTests
  #TestGPURayCastAdditive1.cxx
  TestGPURayCastClipping.cxx
  #TestGPURayCastCropping1.cxx
  #TestGPURayCastDataTypesMinIP1.cxx
  TestGPURayCastGradientOpacity.cxx
  TestGPURayCastVolumePolyData.cxx
  TestGPURayCastVolumeRotation.cxx
  TestGPURayCastVolumeScale.cxx
  )

if ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  list(APPEND OpenGLNew_OR_OpenGL2_CxxTests
    TestGPURayCastVolumeLightKit.cxx
    TestGPURayCastPositionalLights.cxx
    )
endif()

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
  TestProp3DFollower.cxx
  TestTM3DLightComponents.cxx
  ZsweepConcavities.cxx
  volProt.cxx
  )

# everyone gets these tests
vtk_add_test_cxx(${vtk-module}CxxTests tests
  ${GenericVolumeCxxTests}
  )

# If OpenGL2 backend, add all GenericVolumeCxxTests
if (("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2") OR
  (Module_vtkRenderingVolumeOpenGLNew))
  vtk_add_test_cxx(${vtk-module}CxxTests tests
    ${OpenGLNew_OR_OpenGL2_CxxTests}
    )
endif ()

if ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  vtk_test_cxx_executable(${vtk-module}CxxTests tests
    RENDERING_FACTORY
    )
  set_target_properties (${vtk-module}CxxTests
    PROPERTIES COMPILE_FLAGS -DVTK_OPENGL2
    )
  return()
endif ()

vtk_add_test_cxx(${vtk-module}CxxTests tests
  ${VolumeOpenGLCxxTests}
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
