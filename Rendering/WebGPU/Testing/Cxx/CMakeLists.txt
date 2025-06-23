if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(_vtk_test_cxx_wasm_enabled_in_browser ON)
endif ()
# (vtk/vtk#19322): Add baselines for these unit tests
vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  WEBGPU_GRAPHICS_BACKEND
  TestActorFaceCullingProperty.cxx,NO_DATA,NO_SERDES
  TestAxesActor.cxx,NO_DATA,NO_SERDES
  TestCellScalarMappedColors.cxx,NO_DATA,NO_SERDES
  TestCompositePolyDataMapper.cxx,NO_DATA,NO_SERDES
  TestComputeDoublePipelineRenderBuffer.cxx,NO_DATA,NO_SERDES
  TestComputeFrustumCulling.cxx,NO_DATA,NO_VALID
  TestComputeOcclusionCulling.cxx,NO_DATA,NO_VALID
  TestComputeOcclusionCullingResize.cxx,NO_DATA,NO_VALID
  TestComputeModifyPointColors.cxx,NO_DATA,NO_SERDES
  TestComputeModifyCellColors.cxx,NO_DATA,NO_SERDES
  TestComputePass.cxx,NO_DATA,NO_VALID
  TestComputePassChained.cxx,NO_DATA,NO_VALID
  TestComputePassUniform.cxx,NO_DATA,NO_VALID
  TestComputePipelineRelease.cxx,NO_DATA,NO_VALID
  TestComputePointCloudMapper.cxx,NO_DATA,NO_SERDES
  TestComputePointCloudMapperColors.cxx,NO_DATA,NO_SERDES
  TestComputePointCloudMapperDepth.cxx,NO_DATA,NO_SERDES
  TestComputePointCloudMapperGeometry.cxx,NO_DATA,NO_SERDES
  TestComputePointCloudMapperResize.cxx,NO_DATA,NO_SERDES
  TestComputeTexture.cxx,NO_DATA,NO_VALID
  TestFastCellToGraphicsPrimitiveConversion.cxx,NO_DATA,NO_VALID
  TestGlyph3DMapper.cxx,NO_DATA,NO_SERDES
  TestGlyph3DMapperArrow.cxx,NO_SERDES
  TestGlyph3DMapperBounds.cxx,NO_DATA,NO_VALID
  TestGlyph3DMapperCompositeDisplayAttributeInheritance.cxx,NO_DATA,NO_SERDES
  TestGlyph3DMapperIndexing.cxx,NO_DATA,NO_SERDES
  TestGlyph3DMapperMasking.cxx,NO_DATA,NO_SERDES
  TestGlyph3DMapperOrientationArray.cxx,NO_DATA,NO_SERDES
  TestGlyph3DMapperQuaternionArray.cxx,NO_DATA,NO_SERDES
  TestGlyph3DMapperTreeIndexing.cxx,NO_DATA,NO_SERDES
  TestHardwareSelector.cxx,NO_DATA,NO_VALID
  TestLineRendering.cxx,NO_DATA,NO_SERDES
  TestLowPowerRenderWindow.cxx,NO_DATA,NO_SERDES
  TestNActorsNMappersOneInput.cxx,NO_DATA,NO_SERDES
  TestNActorsOneMapper.cxx,NO_DATA,NO_SERDES
  TestNViewportsNActorsNMappersNInputs.cxx,NO_DATA,NO_SERDES
  TestNViewportsNActorsNMappersOneInput.cxx,NO_DATA,NO_SERDES
  TestNViewportsNActorsOneMapper.cxx,NO_DATA,NO_SERDES
  TestNViewportsOneActor.cxx,NO_DATA,NO_SERDES
  TestPointScalarMappedColors.cxx,NO_DATA,NO_SERDES
  TestPolyDataMapper2D.cxx,NO_DATA,NO_SERDES
  TestPolyDataMapperClipPlanes.cxx,NO_DATA,NO_SERDES
  TestReadPixels.cxx,NO_DATA,NO_VALID
  TestRemoveActors.cxx,NO_DATA,NO_VALID
  TestRenderPointsAsSpheres.cxx,NO_DATA,NO_SERDES
  TestRenderLinesAsTubes.cxx,NO_DATA,NO_SERDES
  TestReportWebGPUCapabilities.cxx,NO_DATA,NO_VALID
  TestScalarModeToggle.cxx,NO_DATA,NO_SERDES
  TestSurfacePlusEdges.cxx,NO_DATA,NO_SERDES
  TestTextureInterpolateScalars.cxx
  TestQuad.cxx,NO_DATA,NO_SERDES
  TestQuadPointRep.cxx,NO_DATA,NO_SERDES
  TestRenderWindowChangeDeviceLater.cxx,NO_DATA,NO_SERDES
  TestVertexRendering.cxx,NO_DATA,NO_SERDES
  TestWireframe.cxx,NO_DATA,NO_SERDES)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  WEBGPU_GRAPHICS_BACKEND
  --verify
  TestComputeTriangulationVerifyPointIds,TestComputeTriangulation.cxx,NO_DATA,NO_VALID)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  WEBGPU_GRAPHICS_BACKEND
  --translucent
  TestLineRenderingTranslucent,TestLineRendering.cxx,NO_DATA,NO_SERDES)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  WEBGPU_GRAPHICS_BACKEND
  --point-size 0.5 --line-width 0.5
  TestMixedGeometry_1,TestMixedGeometry.cxx,NO_DATA,NO_SERDES)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  WEBGPU_GRAPHICS_BACKEND
  --point-size 1.0 --line-width 1.0
  TestMixedGeometry_2,TestMixedGeometry.cxx,NO_DATA,NO_SERDES)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  WEBGPU_GRAPHICS_BACKEND
  --point-size 3.0 --line-width 3.0
  TestMixedGeometry_3,TestMixedGeometry.cxx,NO_DATA,NO_SERDES)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  WEBGPU_GRAPHICS_BACKEND
  --point-size 0.5
  TestPointRendering_1,TestPointRendering.cxx,NO_DATA,NO_SERDES)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  WEBGPU_GRAPHICS_BACKEND
  --point-size 1.0
  TestPointRendering_2,TestPointRendering.cxx,NO_DATA,NO_SERDES)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  WEBGPU_GRAPHICS_BACKEND
  --point-size 3.0
  TestPointRendering_3,TestPointRendering.cxx,NO_DATA,NO_SERDES)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  WEBGPU_GRAPHICS_BACKEND
  --point-size 7.0
  TestPointRendering_4,TestPointRendering.cxx,NO_DATA,NO_SERDES)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  WEBGPU_GRAPHICS_BACKEND
  --point-size 0.5 --round
  TestPointRenderingRound_1,TestPointRendering.cxx,NO_DATA,NO_SERDES)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  WEBGPU_GRAPHICS_BACKEND
  --point-size 1.0 --round
  TestPointRenderingRound_2,TestPointRendering.cxx,NO_DATA,NO_SERDES)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  WEBGPU_GRAPHICS_BACKEND
  --point-size 3.0 --round
  TestPointRenderingRound_3,TestPointRendering.cxx,NO_DATA,NO_SERDES)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  WEBGPU_GRAPHICS_BACKEND
  --point-size 7.0 --round
  TestPointRenderingRound_4,TestPointRendering.cxx,NO_DATA,NO_SERDES)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  WEBGPU_GRAPHICS_BACKEND
  --ortho
  TestRenderPointsAsSpheresOrthoCamera,TestRenderPointsAsSpheres.cxx,NO_DATA,NO_SERDES)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  WEBGPU_GRAPHICS_BACKEND
  --ortho
  TestRenderLinesAsTubesOrthoCamera,TestRenderLinesAsTubes.cxx,NO_DATA,NO_SERDES)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  WEBGPU_GRAPHICS_BACKEND
  --point-colors
  TestPolyDataMapper2DPointScalarColorMapping,TestPolyDataMapper2D.cxx,NO_DATA,NO_SERDES)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  WEBGPU_GRAPHICS_BACKEND
  --cell-colors
  TestPolyDataMapper2DCellScalarColorMapping,TestPolyDataMapper2D.cxx,NO_DATA,NO_SERDES)
vtk_test_cxx_executable(vtkRenderingWebGPUCxxTests tests)

if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  # Enable JSPI for webgpu async API
  target_link_options(vtkRenderingWebGPUCxxTests
    PRIVATE
      "-Wno-experimental"
      "-sJSPI=1"
      "-sJSPI_EXPORTS=['__wasm_call_ctors']")
endif ()
