# Tests with data
if(VTK_DATA_ROOT)
  vtk_add_test_cxx(Graphics
    TestArrayCalculator.cxx
    )
endif()

vtk_add_test_cxx(Graphics
  TestGhostArray.cxx
  # TestAppendPolyData.cxx
  TestAppendSelection.cxx
  TestAssignAttribute.cxx
  TestCellDataToPointData.cxx
  TestCenterOfMass.cxx
  TestDecimatePolylineFilter.cxx
  TestDelaunay2D.cxx
  TestExecutionTimer.cxx
  TestGlyph3D.cxx
  TestImplicitPolyDataDistance.cxx
  TestCutter.cxx
  TestThreshold.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests)
