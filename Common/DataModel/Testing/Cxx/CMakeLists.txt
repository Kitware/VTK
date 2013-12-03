vtk_add_test_cxx(NO_DATA NO_VALID NO_OUTPUT
  TestColor.cxx
  TestVector.cxx
  TestVectorOperators.cxx
  TestAMRBox.cxx
  TestCompositeDataSets.cxx
  TestDataArrayDispatcher.cxx
  TestDataObject.cxx
  TestDispatchers.cxx
  TestGenericCell.cxx
  TestGraph.cxx
  TestHigherOrderCell.cxx
  TestImageDataFindCell.cxx
  TestImageDataInterpolation.cxx
  TestImageIterator.cxx
  TestInterpolationDerivs.cxx
  TestInterpolationFunctions.cxx
  TestPath.cxx
  TestPixelExtent.cxx
  TestPointLocators.cxx
  TestPolyDataRemoveCell.cxx
  TestPolygon.cxx
  TestPolyhedron0.cxx
  TestPolyhedron1.cxx
  TestQuadraticPolygon.cxx
  TestSelectionSubtract.cxx
  TestTreeBFSIterator.cxx
  TestTreeDFSIterator.cxx
  TestTriangle.cxx
  otherCellArray.cxx
  otherCellBoundaries.cxx
  otherCellPosition.cxx
  otherCellTypes.cxx
  otherColorTransferFunction.cxx
  otherEmptyCell.cxx
  otherFieldData.cxx
  otherRectilinearGrid.cxx
  otherStructuredGrid.cxx
  quadCellConsistency.cxx
  quadraticEvaluation.cxx
  TestBoundingBox.cxx
  TestPlane.cxx
  TestStructuredData.cxx
  TestDataObjectTypes.cxx
  )

vtk_add_test_cxx(NO_VALID NO_OUTPUT
  TestCellIterators.cxx
  )

vtk_add_test_cxx(
  TestQuadraticPolygonFilters.cxx
  )

vtk_test_cxx_executable(${vtk-module}CxxTests)
