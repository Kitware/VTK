set(cxx_tests
  TestBatchedLabeledDataMapperDisplayCoords.cxx
  TestBatchedLabeledDataMapperWorldCoords.cxx,LOOSE_VALID
  TestClipLabels.cxx
  TestDynamic2DLabelMapper.cxx
  TestLabelPlacementMapper.cxx
  TestLabelPlacementMapperCoincidentPoints.cxx)

if (TARGET VTK::RenderingOpenGL2)
  set(vtk_test_prefix "OpenGL-")
  vtk_add_test_cxx(vtkRenderingLabelCxxTests tests
    --vtk-factory-prefer RenderingBackend=OpenGL
    ${cxx_tests})
  unset(vtk_test_prefix)
endif ()

if (TARGET VTK::RenderingWebGPU)
  set(vtk_test_prefix "WebGPU-")
  vtk_add_test_cxx(vtkRenderingLabelCxxTests tests
    --vtk-factory-prefer RenderingBackend=WebGPU
    ${cxx_tests})
  unset(vtk_test_prefix)
endif ()

vtk_test_cxx_executable(vtkRenderingLabelCxxTests tests)
