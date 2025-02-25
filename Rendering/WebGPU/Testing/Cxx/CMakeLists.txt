if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(_vtk_test_cxx_wasm_enabled_in_browser ON)
endif ()
# (vtk/vtk#19322): Add baselines for these unit tests
vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  TestActorFaceCullingProperty.cxx,NO_DATA
  TestAxesActor.cxx,NO_DATA
  TestCellScalarMappedColors.cxx,NO_DATA
  TestCompositePolyDataMapper.cxx,NO_DATA
  TestComputeDoublePipelineRenderBuffer.cxx,NO_DATA
  TestComputeFrustumCulling.cxx,NO_DATA,NO_VALID
  TestComputeOcclusionCulling.cxx,NO_DATA,NO_VALID
  TestComputeOcclusionCullingResize.cxx,NO_DATA,NO_VALID
  TestComputeModifyPointColors.cxx,NO_DATA
  TestComputeModifyCellColors.cxx,NO_DATA
  TestComputePass.cxx,NO_DATA,NO_VALID
  TestComputePassChained.cxx,NO_DATA,NO_VALID
  TestComputePassUniform.cxx,NO_DATA,NO_VALID
  TestComputePipelineRelease.cxx,NO_DATA,NO_VALID
  TestComputePointCloudMapper.cxx,NO_DATA
  TestComputePointCloudMapperColors.cxx,NO_DATA
  TestComputePointCloudMapperDepth.cxx,NO_DATA
  TestComputePointCloudMapperGeometry.cxx,NO_DATA
  TestComputePointCloudMapperResize.cxx,NO_DATA
  TestComputeTexture.cxx,NO_DATA,NO_VALID
  TestGlyph3DMapper.cxx,NO_DATA
  TestGlyph3DMapperArrow.cxx
  TestGlyph3DMapperBounds.cxx,NO_DATA,NO_VALID
  TestGlyph3DMapperCompositeDisplayAttributeInheritance.cxx,NO_DATA
  TestGlyph3DMapperIndexing.cxx,NO_DATA
  TestGlyph3DMapperMasking.cxx,NO_DATA
  TestGlyph3DMapperOrientationArray.cxx,NO_DATA
  TestGlyph3DMapperQuaternionArray.cxx,NO_DATA
  TestGlyph3DMapperTreeIndexing.cxx,NO_DATA
  TestHardwareSelector.cxx,NO_DATA,NO_VALID
  TestLineRendering.cxx,NO_DATA
  TestLowPowerRenderWindow.cxx,NO_DATA
  TestNActorsNMappersOneInput.cxx,NO_DATA
  TestNActorsOneMapper.cxx,NO_DATA
  TestNViewportsNActorsNMappersNInputs.cxx,NO_DATA
  TestNViewportsNActorsNMappersOneInput.cxx,NO_DATA
  TestNViewportsNActorsOneMapper.cxx,NO_DATA
  TestNViewportsOneActor.cxx,NO_DATA
  TestPointScalarMappedColors.cxx,NO_DATA
  TestPolyDataMapper2D.cxx,NO_DATA
  TestReadPixels.cxx,NO_DATA,NO_VALID
  TestRemoveActors.cxx,NO_DATA,NO_VALID
  TestRenderPointsAsSpheres.cxx,NO_DATA
  TestRenderLinesAsTubes.cxx,NO_DATA
  TestReportWebGPUCapabilities.cxx,NO_DATA,NO_VALID
  TestScalarModeToggle.cxx,NO_DATA
  TestSurfacePlusEdges.cxx,NO_DATA
  TestQuad.cxx,NO_DATA
  TestQuadPointRep.cxx,NO_DATA
  TestRenderWindowChangeDeviceLater.cxx,NO_DATA
  TestVertexRendering.cxx,NO_DATA
  TestWireframe.cxx,NO_DATA)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  --verify
  TestComputeTriangulationVerifyPointIds,TestComputeTriangulation.cxx,NO_DATA,NO_VALID)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  --translucent
  TestLineRenderingTranslucent,TestLineRendering.cxx,NO_DATA)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  --point-size 0.5 --line-width 0.5
  TestMixedGeometry_1,TestMixedGeometry.cxx,NO_DATA)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  --point-size 1.0 --line-width 1.0
  TestMixedGeometry_2,TestMixedGeometry.cxx,NO_DATA)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  --point-size 3.0 --line-width 3.0
  TestMixedGeometry_3,TestMixedGeometry.cxx,NO_DATA)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  --point-size 0.5
  TestPointRendering_1,TestPointRendering.cxx,NO_DATA)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  --point-size 1.0
  TestPointRendering_2,TestPointRendering.cxx,NO_DATA)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  --point-size 3.0
  TestPointRendering_3,TestPointRendering.cxx,NO_DATA)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  --point-size 7.0
  TestPointRendering_4,TestPointRendering.cxx,NO_DATA)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  --point-size 0.5 --round
  TestPointRenderingRound_1,TestPointRendering.cxx,NO_DATA)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  --point-size 1.0 --round
  TestPointRenderingRound_2,TestPointRendering.cxx,NO_DATA)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  --point-size 3.0 --round
  TestPointRenderingRound_3,TestPointRendering.cxx,NO_DATA)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  --point-size 7.0 --round
  TestPointRenderingRound_4,TestPointRendering.cxx,NO_DATA)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  --ortho
  TestRenderPointsAsSpheresOrthoCamera,TestRenderPointsAsSpheres.cxx,NO_DATA)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  --ortho
  TestRenderLinesAsTubesOrthoCamera,TestRenderLinesAsTubes.cxx,NO_DATA)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  --point-colors
  TestPolyDataMapper2DPointScalarColorMapping,TestPolyDataMapper2D.cxx,NO_DATA)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  --cell-colors
  TestPolyDataMapper2DCellScalarColorMapping,TestPolyDataMapper2D.cxx,NO_DATA)
vtk_test_cxx_executable(vtkRenderingWebGPUCxxTests tests)

if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  # Enable asyncify for webgpu async API
  target_link_options(vtkRenderingWebGPUCxxTests
    PRIVATE
      "-sASYNCIFY=1"
      #~297 nesting levels
      "-sASYNCIFY_STACK_SIZE=81920")
endif ()
