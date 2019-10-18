set (VolumeOpenGL2CxxTests
  TestGPURayCastCellData.cxx
  TestGPURayCastChangedArray.cxx
  TestGPURayCastDepthPeeling.cxx
  TestGPURayCastDepthPeelingBoxWidget.cxx
  TestGPURayCastDepthPeelingBlendModes.cxx
  TestGPURayCastDepthPeelingClip.cxx
  TestGPURayCastDepthPeelingOpaque.cxx
  TestGPURayCastDepthPeelingTransVol.cxx
  TestGPURayCastDepthPeelingTransparentPolyData.cxx
  TestGPURayCastIsosurface.cxx
  TestGPURayCastJittering.cxx
  TestGPURayCastSlicePlane.cxx
  TestGPURayCastTextureStreaming.cxx
  TestGPURayCastTextureStreamingMask.cxx
  TestGPURayCastToggleJittering.cxx
  TestGPURayCastUserShader.cxx
  TestGPURayCastUserShader2.cxx
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
  RENDERING_FACTORY
  ${shader_c_files}
  )
