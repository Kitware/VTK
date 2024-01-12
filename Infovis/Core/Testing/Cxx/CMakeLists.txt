set(NEEDS_DATA
  TestStringToNumeric.cxx,NO_VALID
  TestMergeTables.cxx,NO_VALID
  )

vtk_add_test_cxx(vtkInfovisCoreCxxTests tests
  ${NEEDS_DATA}
  ArrayDotProductSimilarity.cxx,NO_VALID,NO_DATA
  ArraySparseArrayToTable.cxx,NO_VALID
  ArrayTableToSparseArray.cxx,NO_VALID
  ArrayToTable.cxx,NO_VALID
  ArrayTransposeMatrix.cxx,NO_VALID
  TestArrayNorm.cxx,NO_VALID,NO_DATA
  TestCollapseVerticesByArray.cxx,NO_VALID
  TestContinuousScatterPlot.cxx,NO_VALID
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
  TestTableToGraph.cxx
  TestThresholdTable.cxx,NO_VALID
  TestTreeDifferenceFilter.cxx,NO_VALID
  TestWordCloud
  UnitTestWordCloud
  )

set(all_tests
  ${tests}
  ${data_tests}
  ${output_tests}
  ${custom_tests}
  )
vtk_test_cxx_executable(vtkInfovisCoreCxxTests all_tests
   DISABLE_FLOATING_POINT_EXCEPTIONS
   RENDERING_FACTORY
   )
