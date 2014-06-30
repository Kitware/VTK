vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestDynamic2DLabelMapper.cxx
  TestLabelPlacer.cxx
  TestLabelPlacer2D.cxx
  TestLabelPlacerCoincidentPoints.cxx
  TestLabelPlacementMapper.cxx
  TestLabelPlacementMapper2D.cxx
  TestLabelPlacementMapperCoincidentPoints.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
