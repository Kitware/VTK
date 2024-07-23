# (vtk/vtk#19322): Run these tests in CI for wasm with _vtk_test_cxx_wasm_enabled_in_browser=1
# (vtk/vtk#19322): Add baselines for these unit tests
vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  TestActorFaceCullingProperty.cxx,NO_DATA
  TestCellScalarMappedColors.cxx,NO_DATA
  # TestCompositePolyDataMapper.cxx,NO_DATA,NO_VALID # FIXME: Composite PolyDataMapper needs updated to delegator design pattern.
  TestComputeDoublePipelineRenderBuffer.cxx,NO_DATA
  TestComputeFrustumCulling.cxx,NO_DATA,NO_VALID
  TestComputeOcclusionCulling.cxx,NO_DATA,NO_VALID
  TestComputeOcclusionCullingResize.cxx,NO_DATA,NO_VALID
  TestComputeModifyPointColors.cxx,NO_DATA
  TestComputeModifyCellColors.cxx,NO_DATA
  TestComputePass.cxx,NO_DATA,NO_VALID
  TestComputePassChained.cxx,NO_DATA,NO_VALID
  TestComputePassUniform.cxx,NO_DATA,NO_VALID
  TestComputeTexture.cxx,NO_DATA,NO_VALID
  TestConesBenchmark.cxx,NO_DATA
  TestLineRendering.cxx,NO_DATA
  TestLowPowerRenderWindow.cxx,NO_DATA
  TestPointScalarMappedColors.cxx,NO_DATA
  TestScalarModeToggle.cxx,NO_DATA
  TestSurfacePlusEdges.cxx,NO_DATA
  TestQuad.cxx,NO_DATA
  TestQuadPointRep.cxx,NO_DATA
  TestRenderWindowChangeDeviceLater.cxx,NO_DATA
  TestVertexRendering.cxx,NO_DATA
  TestWireframe.cxx,NO_DATA)

vtk_test_cxx_executable(vtkRenderingWebGPUCxxTests tests)
