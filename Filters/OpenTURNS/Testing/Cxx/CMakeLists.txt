vtk_add_test_cxx(vtkFiltersOpenTURNSCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestOTDensityMap.cxx
  TestOTKernelSmoothing.cxx
  )

vtk_add_test_cxx(vtkFiltersOpenTURNSCxxTests tests
  TestScatterPlotMatrixVehiclesDensity.cxx)

vtk_test_cxx_executable(vtkFiltersOpenTURNSCxxTests tests)
