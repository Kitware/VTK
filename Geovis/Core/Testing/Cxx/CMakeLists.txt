vtk_add_test_cxx(
  TestGeoAssignCoordinates.cxx
  TestGeoGraticule.cxx
  TestGeoProjection.cxx,NO_VALID
  TestGlobeSource.cxx
  )

if(NOT VTK_REPLACE_OPENGL_OVERRIDES)
  vtk_add_test_cxx(
    TestCoincidentGeoGraphRepresentation2D.cxx
    TestLabeledGeoView2D.cxx
    )
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests)
