if(VTK_DATA_ROOT)
  set (NEEDS_DATA TestStringToNumeric.cxx TestMergeTables.cxx)
endif()

vtk_add_test_cxx(Infovis
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
  )

vtk_test_cxx_executable(${vtk-module}CxxTests)
