vtk_add_test_cxx(
  TestArrayCalculator.cxx,NO_VALID
  )

vtk_add_test_cxx(
  TestGhostArray.cxx,NO_VALID
  # TestAppendPolyData.cxx
  TestAppendSelection.cxx,NO_VALID
  TestAssignAttribute.cxx,NO_VALID
  TestCellDataToPointData.cxx,NO_VALID
  TestCenterOfMass.cxx,NO_VALID
  TestDecimatePolylineFilter.cxx,NO_VALID
  TestDelaunay2D.cxx
  TestExecutionTimer.cxx,NO_VALID
  TestGlyph3D.cxx
  TestImplicitPolyDataDistance.cxx,NO_VALID
  TestCutter.cxx,NO_VALID
  TestThreshold.cxx,NO_VALID
  )
vtk_test_cxx_executable(${vtk-module}CxxTests)
