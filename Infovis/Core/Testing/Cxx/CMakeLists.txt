if(VTK_DATA_ROOT)
  set (NEEDS_DATA TestStringToNumeric.cxx TestMergeTables.cxx)
endif()

create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
  ${NEEDS_DATA}
  ArraySparseArrayToTable.cxx
  ArrayTableToSparseArray.cxx
  ArrayToTable.cxx
  ArrayTransposeMatrix.cxx
  # TestArrayNorm.cxx             # todo (unsatisfied deps)
  TestCollapseVerticesByArray.cxx
  # TestConvexHull2D.cxx          # todo (unsatisfied deps)
  TestDataObjectToTable.cxx
  TestExtractSelectedTree.cxx
  TestExtractSelectedGraph.cxx
  TestGraphAlgorithms.cxx
  TestMergeGraphs.cxx
  TestPruneTreeFilter.cxx
  TestRandomGraphSource.cxx
  TestRemoveIsolatedVertices.cxx
  TestStreamGraph.cxx
  TestTableToArray.cxx
  TestThresholdTable.cxx
  TestTreeDifferenceFilter.cxx
  # TestTimePoint.cxx             # deprecated dep

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
        -V Baseline/Infovis/${TName}.png)
  else()
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach()
