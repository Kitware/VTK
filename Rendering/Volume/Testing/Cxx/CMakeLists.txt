set (VolumeCxxTests
  ProjectedTetrahedraZoomIn.cxx,NO_VALID
  TestFinalColorWindowLevel.cxx
  TestFixedPointRayCastLightComponents.cxx
  TestGPURayCastAdditive.cxx
  TestGPURayCastAverageIP.cxx
  TestGPURayCastBlendModes.cxx
  TestGPURayCastCameraInside.cxx
  TestGPURayCastCameraInsideClipping.cxx
  TestGPURayCastCameraInsideNonUniformScaleTransform.cxx
  TestGPURayCastCameraInsideSmallSpacing.cxx
  TestGPURayCastCameraInsideTransformation.cxx
  TestGPURayCastCellData.cxx
  TestGPURayCastClipping.cxx
  TestGPURayCastClippingPolyData.cxx
  TestGPURayCastClippingUserTransform.cxx
  TestGPURayCastCompositeBinaryMask.cxx
  TestGPURayCastCompositeBinaryMask1.cxx
  TestGPURayCastCompositeMask.cxx
  TestGPURayCastCompositeMaskBlend.cxx
  TestGPURayCastCompositeToMIP.cxx
  TestGPURayCastCropping.cxx
  TestGPURayCastDataTypesMIP.cxx
  TestGPURayCastDataTypesMinIP.cxx
  TestGPURayCastDependentComponentsLightParameters.cxx
  TestGPURayCastDepthPeeling.cxx
  TestGPURayCastDepthPeelingBlendModes.cxx
  TestGPURayCastDepthPeelingClip.cxx
  TestGPURayCastDepthPeelingOpaque.cxx
  TestGPURayCastDepthPeelingTransVol.cxx
  TestGPURayCastDepthPeelingTransparentPolyData.cxx
  TestGPURayCastFourComponentsAdditive.cxx
  TestGPURayCastFourComponentsAverage.cxx
  TestGPURayCastFourComponentsComposite.cxx
  TestGPURayCastFourComponentsCompositeStreaming.cxx
  TestGPURayCastFourComponentsDependentGradient.cxx
  TestGPURayCastFourComponentsMIP.cxx
  TestGPURayCastFourComponentsMinIP.cxx
  TestGPURayCastGradientOpacity.cxx
  TestGPURayCastGradientOpacityLight.cxx
  TestGPURayCastImageSampleXY.cxx
  TestGPURayCastIndependentComponentsLightParameters.cxx
  TestGPURayCastIndependentVectorMode.cxx
  TestGPURayCastIsosurface.cxx
  TestGPURayCastJittering.cxx
  TestGPURayCastJitteringCustom.cxx
  TestGPURayCastLargeColorTransferFunction.cxx
  TestGPURayCastMIPBinaryMask.cxx
  TestGPURayCastMIPToComposite.cxx
  TestGPURayCastMapperBenchmark.cxx
  TestGPURayCastMapperSampleDistance.cxx
  TestGPURayCastMultiVolumeAddRemove.cxx
  TestGPURayCastMultiVolumeCellData.cxx
  TestGPURayCastMultiVolumeOverlapping.cxx
  TestGPURayCastMultiVolumeTransfer2D.cxx
  TestGPURayCastNearestDataTypesMIP.cxx
  TestGPURayCastPerspectiveParallel.cxx
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
  TestGPURayCastVolumeOrientation.cxx
  TestGPURayCastVolumePicking.cxx
  TestGPURayCastVolumePlane.cxx
  TestGPURayCastVolumePolyData.cxx
  TestGPURayCastVolumeRotation.cxx
  TestGPURayCastVolumeScale.cxx
  TestGPURayCastVolumeUpdate.cxx
  TestGPUVolumeRayCastMapper.cxx
  TestMinIntensityRendering.cxx
  TestMultiBlockMapper.cxx
  TestProjectedTetrahedra.cxx
  TestProjectedTetrahedraOffscreen.cxx
  TestProjectedTetrahedraTransform.cxx
  TestRemoveVolumeNonCurrentContext.cxx
  TestSmartVolumeMapper.cxx
  TestSmartVolumeMapperWindowLevel.cxx
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
vtk_add_test_cxx(vtkRenderingVolumeCxxTests tests
  ${VolumeCxxTests}
  )

vtk_test_cxx_executable(vtkRenderingVolumeCxxTests tests
  RENDERING_FACTORY
  ${shader_c_files}
  )
