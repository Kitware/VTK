# Generic tests
vtk_add_test_cxx(vtkRenderingContext2DCxxTests tests
  TestContext2D.cxx,NO_VALID
  TestPolyDataToContext.cxx
  )

vtk_test_cxx_executable(vtkRenderingContext2DCxxTests tests)
