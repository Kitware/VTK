vtk_add_test_cxx(vtkCommonComputationalGeometryCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  UnitTestParametricSpline.cxx
  TestBilinearQuadIntersection.cxx
  )
vtk_test_cxx_executable(vtkCommonComputationalGeometryCxxTests tests)
