vtk_add_test_cxx(vtkRenderingLabelCxxTests tests
  TestClipLabels.cxx
  TestDynamic2DLabelMapper.cxx
  TestLabelPlacer.cxx
  TestLabelPlacer2D.cxx
  TestLabelPlacerCoincidentPoints.cxx
  TestLabelPlacementMapper.cxx
  TestLabelPlacementMapper2D.cxx
  TestLabelPlacementMapperCoincidentPoints.cxx
  )
vtk_test_cxx_executable(vtkRenderingLabelCxxTests tests)
