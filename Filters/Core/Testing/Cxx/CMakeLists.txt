# Tests with data
if(VTK_DATA_ROOT)
  set(NEEDS_DATA
    TestArrayCalculator
    )
endif()

create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
  ${NEEDS_DATA}
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

  EXTRA_INCLUDE vtkTestDriver.h)

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
        -V Baseline/Graphics/${TName}.png)
  else()
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach()