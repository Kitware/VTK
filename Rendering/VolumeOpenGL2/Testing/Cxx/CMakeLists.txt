# Only tests that reach for something the OpenGL backend alone provides belong here. Anything
# that talks to vtkGPUVolumeRayCastMapper through its base class lives in VTK::RenderingVolume,
# where it is run against every backend.
set (VolumeOpenGL2CxxTests
  TestGPURayCastCellData.cxx
  TestGPURayCastDepthPeeling.cxx
  TestGPURayCastDepthPeelingMultiViewport.cxx
  TestGPURayCastDepthPeelingBoxWidget.cxx
  TestGPURayCastDepthPeelingBlendModes.cxx
  TestGPURayCastDepthPeelingClip.cxx
  TestGPURayCastDepthPeelingOpaque.cxx
  TestGPURayCastDepthPeelingOpaqueBackdrop.cxx,NO_DATA
  TestGPURayCastDepthPeelingTransVol.cxx
  TestGPURayCastDepthPeelingTransparentPolyData.cxx
  TestGPURayCastDepthTextureFormat.cxx
  TestGPURayCastJittering.cxx
  TestGPURayCastSSAO.cxx
  TestGPURayCastTextureStreaming.cxx
  TestGPURayCastTextureStreamingMask.cxx
  TestGPURayCastToggleJittering.cxx
  TestGPURayCastUserShader.cxx
  TestGPURayCastUserShader2.cxx
  TestSurfaceProbeVolumeMapper.cxx
  TestSurfaceProbeVolumeMapperColoring.cxx
  )

# everyone gets these tests
vtk_add_test_cxx(vtkRenderingVolumeOpenGL2CxxTests tests
  ${VolumeOpenGL2CxxTests}
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


vtk_test_cxx_executable(vtkRenderingVolumeOpenGL2CxxTests tests
  DISABLE_FLOATING_POINT_EXCEPTIONS
  RENDERING_FACTORY
  ${shader_c_files}
  )
