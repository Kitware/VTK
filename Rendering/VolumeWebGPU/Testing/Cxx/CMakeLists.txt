# These exercise what only the WebGPU mapper offers, so there is one backend to ask for.
vtk_add_test_cxx(vtkRenderingVolumeWebGPUCxxTests tests
  --vtk-factory-prefer RenderingBackend=WebGPU
  TestWebGPUVolumeAdaptiveSampling.cxx,NO_DATA,NO_VALID
  TestWebGPUVolumeBlendModes.cxx,NO_DATA,NO_VALID
  TestWebGPUVolumeCameraInside.cxx,NO_DATA,NO_VALID
  TestWebGPUVolumeMixedGeometry.cxx,NO_DATA,NO_VALID
  TestWebGPUVolumeOverlap.cxx,NO_DATA,NO_VALID
  TestWebGPUVolumeRayCast.cxx,NO_DATA,NO_VALID)

vtk_test_cxx_executable(vtkRenderingVolumeWebGPUCxxTests tests
  RENDERING_FACTORY
  )
