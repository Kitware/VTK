# The mapper only talks to the ray cast mappers through their base classes, so each backend that
# is built runs the list.
set(AMRVolumeCxxTests
  TestAMRVolumeMapper.cxx,NO_DATA,NO_SERDES)

if (TARGET VTK::RenderingVolumeOpenGL2)
  set(vtk_test_prefix "OpenGL-")
  vtk_add_test_cxx(vtkRenderingVolumeAMRCxxTests tests
    --vtk-factory-prefer RenderingBackend=OpenGL
    ${AMRVolumeCxxTests})
  unset(vtk_test_prefix)
endif ()

if (TARGET VTK::RenderingVolumeWebGPU)
  set(vtk_test_prefix "WebGPU-")
  vtk_add_test_cxx(vtkRenderingVolumeAMRCxxTests tests
    --vtk-factory-prefer RenderingBackend=WebGPU
    ${AMRVolumeCxxTests})
  unset(vtk_test_prefix)
endif ()

vtk_test_cxx_executable(vtkRenderingVolumeAMRCxxTests tests
  DISABLE_FLOATING_POINT_EXCEPTIONS
  RENDERING_FACTORY)
