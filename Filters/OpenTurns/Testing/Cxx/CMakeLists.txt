vtk_add_test_cxx(vtkFiltersOpenTurnsCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestOTDensityMap.cxx
  TestOTKernelSmoothing.cxx
  )

vtk_add_test_cxx(vtkFiltersOpenTurnsCxxTests tests
  TestScatterPlotMatrixVehiclesDensity.cxx)

vtk_test_cxx_executable(vtkFiltersOpenTurnsCxxTests tests)
