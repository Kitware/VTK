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
  TestGPURayCastChangedArray.cxx
  TestGPURayCastClipping.cxx
  TestGPURayCastClippingPolyData.cxx
  TestGPURayCastClippingUserTransform.cxx
  TestGPURayCastCompositeBinaryMask.cxx
  TestGPURayCastCompositeBinaryMask1.cxx
  TestGPURayCastCompositeMask.cxx
  TestGPURayCastCompositeMaskBlend.cxx
  TestGPURayCastCompositeToMIP.cxx
  TestGPURayCastComputeNormalFromOpacity.cxx
  TestGPURayCastCropping.cxx
  TestGPURayCastCropping1.cxx
  TestGPURayCastDataTypesMIP.cxx
  TestGPURayCastDataTypesMinIP.cxx
  TestGPURayCastDependentComponentsLightParameters.cxx
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
  TestGPURayCastLabelMap.cxx
  TestGPURayCastLabelMap1Label.cxx
  TestGPURayCastLabelMapValidity.cxx
  TestGPURayCastLargeColorTransferFunction.cxx
  TestGPURayCastMIPBinaryMask.cxx
  TestGPURayCastMIPToComposite.cxx
  TestGPURayCastMapperBenchmark.cxx
  TestGPURayCastMapperRectilinearGrid.cxx
  TestGPURayCastMapperSampleDistance.cxx
  TestGPURayCastMapperShadows.cxx
  TestGPURayCastModelTransformMatrix.cxx
  TestGPURayCastMultiVolumeAddRemove.cxx
  TestGPURayCastMultiVolumeCellData.cxx
  TestGPURayCastMultiVolumeClipping.cxx
  TestGPURayCastMultiVolumeOverlapping.cxx
  TestGPURayCastMultiVolumeRGBA.cxx
  TestGPURayCastMultiVolumeShade.cxx
  TestGPURayCastMultiVolumeTransfer2D.cxx
  TestGPURayCastNearestDataTypesMIP.cxx
  TestGPURayCastOrientedVolume.cxx
  TestGPURayCastPerspectiveParallel.cxx
  TestGPURayCastPositionalLights.cxx
  TestGPURayCastReleaseResources.cxx
  TestGPURayCastRenderDepthToImage.cxx
  TestGPURayCastRenderDepthToImage2.cxx
  TestGPURayCastRenderToTexture.cxx
  TestGPURayCastShadedClipping.cxx
  TestGPURayCastSlicePlane.cxx
  TestGPURayCastThreeComponentsAdditive.cxx
  TestGPURayCastThreeComponentsIndependent.cxx
  TestGPURayCastTransfer2D.cxx
  TestGPURayCastTransfer2DYScalars.cxx
  TestGPURayCastTwoComponentsDependent.cxx
  TestGPURayCastTwoComponentsDependentGradient.cxx
  TestGPURayCastTwoComponentsGradient.cxx
  TestGPURayCastTwoComponentsIndependent.cxx
  TestGPURayCastVolumeDepthPass.cxx
  TestGPURayCastVolumeGhostArrays.cxx
  TestGPURayCastVolumeGhostArraysCellData.cxx
  TestGPURayCastVolumeLightKit.cxx
  TestGPURayCastVolumeOrientation.cxx
  TestGPURayCastVolumePicking.cxx
  TestGPURayCastVolumePlane.cxx
  TestGPURayCastVolumePolyData.cxx
  TestGPURayCastVolumeRotation.cxx
  TestGPURayCastVolumeScale.cxx
  TestGPURayCastVolumeUniformGridBlanking.cxx
  TestGPURayCastVolumeUpdate.cxx
  TestGPUVolumeRayCastMapper.cxx
  TestGPUVolumeRayCastMapperManyLights.cxx
  TestMinIntensityRendering.cxx
  TestMultiBlockMapper.cxx
  TestMultiBlockMapperRectilinearGrid.cxx
  TestMultiBlockUnstructuredGridVolumeMapper.cxx
  TestProjectedTetrahedra.cxx
  TestProjectedTetrahedraOffscreen.cxx
  TestProjectedTetrahedraTransform.cxx
  TestProjectedTetrahedraVectorComponent.cxx
  TestRemoveVolumeNonCurrentContext.cxx
  TestSmartVolumeMapper.cxx
  TestSmartVolumeMapperImplicitArray.cxx,NO_SERDES
  TestSmartVolumeMapperVolumeUpdate.cxx
  TestSmartVolumeMapperWindowLevel.cxx
  )

# The tests above only talk to the mappers through their base classes, so each backend that is
# built runs the whole list. The rendering test driver turns --vtk-factory-prefer into a
# vtkObjectFactory preference, which is what picks the mapper and the render window.
if (TARGET VTK::RenderingVolumeOpenGL2)
  set(vtk_test_prefix "OpenGL-")
  vtk_add_test_cxx(vtkRenderingVolumeCxxTests tests
    --vtk-factory-prefer RenderingBackend=OpenGL
    ${VolumeCxxTests}
    )
  unset(vtk_test_prefix)
endif ()

if (TARGET VTK::RenderingVolumeWebGPU)
  set(vtk_test_prefix "WebGPU-")
  vtk_add_test_cxx(vtkRenderingVolumeCxxTests tests
    --vtk-factory-prefer RenderingBackend=WebGPU
    ${VolumeCxxTests}
    )
  unset(vtk_test_prefix)
endif ()

vtk_test_cxx_executable(vtkRenderingVolumeCxxTests tests
  DISABLE_FLOATING_POINT_EXCEPTIONS
  RENDERING_FACTORY
  )

vtk_module_test_data(
  Data/TestGPURayCastTransfer2DYScalarsTransferFunction.vti
  Data/disk_out_ref.ex2
  )
