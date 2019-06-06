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
  )

# add to the list but don't define a test
list(APPEND tests UnitTestWordCloud.cxx)
list(APPEND tests TestWordCloud.cxx)

ExternalData_add_test(${_vtk_build_TEST_DATA_TARGET}
  NAME VTK::InfovisCoreCxxTests-UnitTestWordCloud
  COMMAND vtkInfovisCoreCxxTests UnitTestWordCloud
  DATA{../../../../Testing/Data/Gettysburg.txt} DATA{../../../../Testing/Data/Canterbury.ttf} DATA{../../../../Testing/Data/hearts.png} DATA{../../../../Testing/Data/hearts8bit.png}  DATA{../../../../Testing/Data/NLTKStopList.txt})

ExternalData_add_test(${_vtk_build_TEST_DATA_TARGET}
  NAME VTK::InfovisCoreCxxTests-TestWordCloud
  COMMAND vtkInfovisCoreCxxTests TestWordCloud
  DATA{../../../../Testing/Data/Gettysburg.txt} DATA{../../../../Testing/Data/Canterbury.ttf}
  -V DATA{../Data/Baseline/TestWordCloud.png,:}
  -T "${VTK_TEST_OUTPUT_DIR}"
)

set(all_tests
  ${tests}
  ${data_tests}
  ${output_tests}
  ${custom_tests}
  )
vtk_test_cxx_executable(vtkInfovisCoreCxxTests all_tests RENDERING_FACTORY)
