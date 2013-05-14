vtk_add_test_cxx(
  TestCoincidentGeoGraphRepresentation2D.cxx
  TestGeoAssignCoordinates.cxx
  TestGeoGraticule.cxx
  TestGeoProjection.cxx,NO_VALID
  TestGlobeSource.cxx
  TestLabeledGeoView2D.cxx
  )

vtk_test_cxx_executable(${vtk-module}CxxTests)
