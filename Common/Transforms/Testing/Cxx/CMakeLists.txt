vtk_add_test_cxx(vtkCommonTransformsCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestTransform.cxx
  TestLandmarkTransform.cxx
  TestThinPlateSplineTransform.cxx
  )
vtk_test_cxx_executable(vtkCommonTransformsCxxTests tests)
