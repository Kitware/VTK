if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(_vtk_test_cxx_wasm_enabled_in_browser ON)
endif ()
# (vtk/vtk#19322): Add baselines for these unit tests
vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  TestComputeDoublePipelineRenderBuffer.cxx,NO_DATA
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
  TestLowPowerRenderWindow.cxx,NO_DATA,NO_SERDES
  TestReportWebGPUCapabilities.cxx,NO_DATA,NO_VALID
  TestRenderWindowChangeDeviceLater.cxx,NO_DATA,NO_SERDES)

vtk_add_test_cxx(vtkRenderingWebGPUCxxTests tests
  --verify
  TestComputeTriangulationVerifyPointIds,TestComputeTriangulation.cxx,NO_DATA,NO_VALID)

vtk_test_cxx_executable(vtkRenderingWebGPUCxxTests tests)

if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  # Enable JSPI for webgpu async API
  target_link_options(vtkRenderingWebGPUCxxTests
    PRIVATE
      "-Wno-experimental"
      "-sJSPI=1"
      "-sJSPI_EXPORTS=['__wasm_call_ctors']")
endif ()
