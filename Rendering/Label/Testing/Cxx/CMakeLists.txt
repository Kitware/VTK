vtk_add_test_cxx(vtkRenderingLabelCxxTests tests
  TestClipLabels.cxx
  TestDynamic2DLabelMapper.cxx
  TestLabelPlacer.cxx
  TestLabelPlacer2D.cxx
  TestLabelPlacerCoincidentPoints.cxx,LOOSE_VALID
  TestLabelPlacementMapper.cxx
  TestLabelPlacementMapper2D.cxx
  TestLabelPlacementMapperCoincidentPoints.cxx
  )
vtk_test_cxx_executable(vtkRenderingLabelCxxTests tests)
set_tests_properties(VTK::RenderingLabelCxx-TestLabelPlacer PROPERTIES TIMEOUT 300)
