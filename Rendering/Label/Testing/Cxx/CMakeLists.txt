set(cxx_tests
  TestBatchedLabeledDataMapperDisplayCoords.cxx
  TestBatchedLabeledDataMapperWorldCoords.cxx,LOOSE_VALID
  TestClipLabels.cxx
  TestDynamic2DLabelMapper.cxx
  TestLabelPlacer.cxx
  TestLabelPlacer2D.cxx
  TestLabelPlacerCoincidentPoints.cxx,LOOSE_VALID
  TestLabelPlacementMapper.cxx
  TestLabelPlacementMapper2D.cxx
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
if (TARGET VTK::RenderingOpenGL2)
  set_tests_properties(VTK::RenderingLabelCxx-OpenGL-TestLabelPlacer PROPERTIES TIMEOUT 300)
endif ()
if (TARGET VTK::RenderingWebGPU)
  set_tests_properties(VTK::RenderingLabelCxx-WebGPU-TestLabelPlacer PROPERTIES TIMEOUT 300)
endif ()
