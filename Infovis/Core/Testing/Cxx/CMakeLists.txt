set (NEEDS_DATA
  TestStringToNumeric.cxx,NO_VALID
  TestMergeTables.cxx,NO_VALID
  )

vtk_add_test_cxx(
  ${NEEDS_DATA}
  ArraySparseArrayToTable.cxx,NO_VALID
  ArrayTableToSparseArray.cxx,NO_VALID
  ArrayToTable.cxx,NO_VALID
  ArrayTransposeMatrix.cxx,NO_VALID
  # TestArrayNorm.cxx             # todo (unsatisfied deps)
  TestCollapseVerticesByArray.cxx,NO_VALID
  # TestConvexHull2D.cxx          # todo (unsatisfied deps)
  TestDataObjectToTable.cxx,NO_VALID
  TestExtractSelectedTree.cxx,NO_VALID
  TestExtractSelectedGraph.cxx,NO_VALID
  TestGraphAlgorithms.cxx
  TestMergeGraphs.cxx,NO_VALID
  TestPruneTreeFilter.cxx
  TestRandomGraphSource.cxx,NO_VALID
  TestReduceTable.cxx,NO_VALID
  TestRemoveIsolatedVertices.cxx,NO_VALID
  TestStreamGraph.cxx,NO_VALID
  TestTableToArray.cxx,NO_VALID
  TestThresholdTable.cxx,NO_VALID
  TestTreeDifferenceFilter.cxx,NO_VALID
  # TestTimePoint.cxx             # deprecated dep
  )

vtk_test_cxx_executable(${vtk-module}CxxTests)
