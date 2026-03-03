vtk_add_test_cxx(vtkFiltersMeshingCxxTests tests
  TestGeneralizedSurfaceNets3D.cxx
  TestShellBinIterator.cxx,NO_VALID
  TestLabeledImagePointSampler.cxx,
  TestLabeledImagePointSampler2.cxx,
  TestLabeledImagePointSampler3.cxx,
  TestVoronoi2DSurfaceNets.cxx,
  TestVoronoiCore2D.cxx,NO_VALID
  TestVoronoiCore3D.cxx,NO_VALID
  TestVoronoiHull2.cxx
  TestVoronoi3DMapScalars.cxx
  )

set(all_tests
  ${tests}
  )

vtk_test_cxx_executable(vtkFiltersMeshingCxxTests all_tests)
