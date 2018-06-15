set(Deprecated_Tests)
if(NOT VTK_LEGACY_REMOVE)
  list(APPEND Deprecated_Tests
    TestGeoAssignCoordinates.cxx
    TestGeoGraticule.cxx
    TestGlobeSource.cxx
    )
endif()

vtk_add_test_cxx(vtkGeovisCoreCxxTests tests
  TestGeoProjection.cxx,NO_VALID
  ${Deprecated_Tests}
  )

if(NOT VTK_REPLACE_OPENGL_OVERRIDES AND
  NOT VTK_LEGACY_REMOVE)
  vtk_add_test_cxx(vtkGeovisCoreCxxTests tests
    TestCoincidentGeoGraphRepresentation2D.cxx
    TestLabeledGeoView2D.cxx
    )
endif()

vtk_test_cxx_executable(vtkGeovisCoreCxxTests tests)
