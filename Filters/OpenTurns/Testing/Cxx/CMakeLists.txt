vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestOTDensityMap.cxx
  TestOTKernelSmoothing.cxx
  )

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestScatterPlotMatrixVehiclesDensity.cxx)

vtk_test_cxx_executable(${vtk-module}CxxTests tests)
