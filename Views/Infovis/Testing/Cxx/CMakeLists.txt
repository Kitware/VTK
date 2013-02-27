set(MyTests
  TestCoincidentGraphLayoutView.cxx
  TestConeLayoutStrategy.cxx
  TestDendrogramOnly.cxx
  TestGraphLayoutView.cxx
  TestHeatmapOnly.cxx
  TestHierarchicalGraphView.cxx
  TestIcicleView.cxx
  TestInteractorStyleTreeMapHover.cxx
  TestNetworkViews.cxx
  TestRenderView.cxx
  TestSpanTreeLayoutStrategy.cxx
  TestTreeHeatmapAutoCollapse.cxx
  TestTreeHeatmapItem.cxx
  TestTreeMapView.cxx
  TestTreeRingView.cxx
  ${QtTests}
)

if( VTK_DATA_ROOT)
  set(MyTests ${MyTests} TestIconGlyphFilter.cxx)
endif()

# Use the testing object factory, to reduce boilerplate code in tests.
include(vtkTestingObjectFactory)
vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun CxxTests.cxx)

# Add all the executables
foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  if(VTK_DATA_ROOT)
    add_test(NAME ${vtk-module}Cxx-${TName}
            COMMAND ${vtk-module}CxxTests ${TName}
                                          -D ${VTK_DATA_ROOT}
                                          -T ${VTK_TEST_OUTPUT_DIR}
                                          -V Baseline/Views/${TName}.png)
  else()
    add_test(NAME ${vtk-module}Cxx-${TName} COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach()
