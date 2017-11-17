# These tests are built under all conditions
set (GenericVolumeCxxTests
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
  TestGPURayCastTextureStreamingMask.cxx
  TestGPURayCastThreeComponentsAdditive.cxx
  TestGPURayCastThreeComponentsIndependent.cxx
  TestGPURayCastTransfer2D.cxx
  TestGPURayCastTwoComponentsDependent.cxx
  TestGPURayCastTwoComponentsDependentGradient.cxx
  TestGPURayCastTwoComponentsGradient.cxx
  TestGPURayCastTwoComponentsIndependent.cxx
  TestGPURayCastUserShader.cxx
  TestGPURayCastUserShader2.cxx
  TestGPURayCastVolumeDepthPass.cxx
  TestGPURayCastVolumeLightKit.cxx
  TestGPURayCastVolumePicking.cxx
  TestGPURayCastVolumePlane.cxx
  TestGPURayCastVolumePolyData.cxx
  TestGPURayCastVolumeRotation.cxx
  TestGPURayCastVolumeScale.cxx
  TestMultiBlockMapper.cxx
  TestProjectedTetrahedraOffscreen.cxx
  TestProjectedTetrahedraTransform.cxx
  TestRemoveVolumeNonCurrentContext.cxx
  )

set(test_shader_files
  TestGPURayCastUserShader2_FS.glsl
  )
unset(shader_c_files)
foreach(file ${test_shader_files})
  vtk_encode_string(
    INPUT         "${file}"
    SOURCE_OUTPUT source)
  list(APPEND shader_c_files ${source})
endforeach()

include_directories(${CMAKE_CURRENT_BINARY_DIR})

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

vtk_test_cxx_executable(${vtk-module}CxxTests tests
  RENDERING_FACTORY
  ${shader_c_files}
  )
