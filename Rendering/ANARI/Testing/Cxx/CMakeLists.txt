vtk_add_test_cxx(vtkRenderingAnariCxxTests tests
  # Surface Rendering Tests
  TestAnariPass.cxx
  TestAnariPassVisibility.cxx
  TestAnariMaterials.cxx
  TestAnariSphere.cxx
  TestAnariDynamicObject.cxx
  TestAnariAmbient.cxx
  TestAnariCompositePolyDataMapper.cxx
  TestAnariCylinder.cxx
  TestAnariImplicits.cxx
  TestAnariLayers.cxx
  TestAnariLights.cxx
  TestAnariMultiBlock.cxx
  TestAnariMultiBlockPartialArrayFieldData.cxx
  TestAnariOrthographic.cxx
  TestAnariRenderMesh.cxx
  TestAnariRendererType.cxx
  TestAnariScalarBar.cxx
  TestAnariStereo.cxx
  TestAnariWindow.cxx
  TestAnariShadows.cxx
  TestAnariPBRMaterials.cxx
  TestAnariPBRMapping.cxx
  TestAnariPBRMaterialsCoat.cxx
  # Volume Rendering Tests
  TestAnariVolumeRenderer.cxx
  TestAnariSmartVolumeMapper.cxx
  TestAnariVolumeWavelet.cxx
  TestAnariTransfer2D.cxx
  TestAnariCameraInside.cxx
  TestAnariCameraInsideSmallSpacing.cxx
  TestAnariCellData.cxx
  TestAnariVolumeLightKit.cxx
  TestAnariPerspectiveParallel.cxx
  TestAnariPositionalLights.cxx)

vtk_test_cxx_executable(vtkRenderingAnariCxxTests tests RENDERING_FACTORY
  DISABLE_FLOATING_POINT_EXCEPTIONS
  vtkAnariTestInteractor.cxx
  )
