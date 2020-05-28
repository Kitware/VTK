vtk_add_test_cxx(vtkGeovisCoreCxxTests tests
  TestGeoAssignCoordinates.cxx
  TestGeoGraticule.cxx
  TestGeoProjection.cxx,NO_VALID
  TestGlobeSource.cxx
  )

vtk_test_cxx_executable(vtkGeovisCoreCxxTests tests)
