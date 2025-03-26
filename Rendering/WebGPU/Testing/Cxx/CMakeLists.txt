# (vtk/vtk#19322): Run these tests in CI for wasm with _vtk_test_cxx_wasm_enabled_in_browser=1
# (vtk/vtk#19322): Add baselines for these unit tests
vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  TestActorFaceCullingProperty.cxx,NO_DATA
  TestCellScalarMappedColors.cxx,NO_DATA,NO_VALID
  TestCompositePolyDataMapper.cxx,NO_DATA,NO_VALID
  TestComputeDoublePipelineRenderBuffer.cxx,NO_DATA
  TestComputeModifyPointColors.cxx,NO_DATA
  TestComputeModifyCellColors.cxx,NO_DATA
  TestComputePipeline.cxx,NO_DATA_,NO_VALID
  TestComputePipelineUniform.cxx,NO_DATA_,NO_VALID
  TestConesBenchmark.cxx,NO_DATA,NO_VALID
  TestLineRendering.cxx,NO_DATA,NO_VALID
  TestPointScalarMappedColors.cxx,NO_DATA,NO_VALID
  TestScalarModeToggle.cxx,NO_DATA,NO_VALID
  TestSurfacePlusEdges.cxx,NO_DATA,NO_VALID
  TestTheQuad.cxx,NO_DATA,NO_VALID
  TestTheQuadPointRepresentation.cxx,NO_DATA,NO_VALID
  TestTheQuadWireframeRepresentation.cxx,NO_DATA,NO_VALID
  TestTheTriangle.cxx,NO_DATA,NO_VALID
  TestTheTrianglePointRepresentation.cxx,NO_DATA,NO_VALID
  TestTheTriangleWireframeRepresentation.cxx,NO_DATA,NO_VALID
  TestVertexRendering.cxx,NO_DATA,NO_VALID
  TestWireframe.cxx,NO_DATA,NO_VALID)

vtk_test_cxx_executable(vtkRenderingWebGPUCxxTests tests)
