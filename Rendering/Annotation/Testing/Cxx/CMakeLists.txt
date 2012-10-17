create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
  # TestAxisActor3D.cxx # todo (may not be in vtk in the first place)
  TestBarChartActor.cxx
  TestCubeAxes2DMode.cxx
  TestCubeAxes3.cxx
  TestCubeAxesWithGridLines.cxx
  TestCubeAxesWithXInnerGrids.cxx
  TestCubeAxesWithXInnerPolys.cxx
  TestCubeAxesWithXLines.cxx
  TestCubeAxesWithYInnerGrids.cxx
  TestCubeAxesWithYInnerPolys.cxx
  TestCubeAxesWithYLines.cxx
  TestCubeAxesWithZInnerGrids.cxx
  TestCubeAxesWithZInnerPolys.cxx
  TestCubeAxesWithZLines.cxx
  TestLegendBoxActor.cxx
  TestLegendScaleActor.cxx
  TestPieChartActor.cxx
  TestPolarAxes.cxx
  TestSpiderPlotActor.cxx
  TestCubeAxesOrientedBoundingBox.cxx
  TestCubeAxesIntersectionPoint.cxx
  TestCubeAxesInnerGridAll.cxx
  TestCubeAxesInnerGridClosest.cxx
  TestCubeAxesInnerGridFurthest.cxx
  TestXYPlotActor.cxx

  EXTRA_INCLUDE vtkTestDriver.h
)

vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun ${vtk-module}CxxTests.cxx)

# Add all the executables
foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  if(VTK_DATA_ROOT)
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/Hybrid/${TName}.png)
  else()
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach()
