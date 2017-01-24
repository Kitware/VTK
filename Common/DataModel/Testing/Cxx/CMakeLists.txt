vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestColor.cxx
  TestVector.cxx
  TestVectorOperators.cxx
  TestAMRBox.cxx
  TestBiQuadraticQuad.cxx
  TestCompositeDataSets.cxx
  TestComputeBoundingSphere.cxx
  TestDataArrayDispatcher.cxx
  TestDataObject.cxx
  TestDispatchers.cxx
  TestGenericCell.cxx
  TestGraph.cxx
  TestGraph2.cxx
  TestGraphAttributes.cxx
  TestHigherOrderCell.cxx
  TestImageDataFindCell.cxx
  TestImageDataInterpolation.cxx
  TestImageIterator.cxx
  TestInterpolationDerivs.cxx
  TestInterpolationFunctions.cxx
  TestPath.cxx
  TestPentagonalPrism.cxx
  TestPixelExtent.cxx
  TestPointLocators.cxx
  TestPolyDataRemoveCell.cxx
  TestPolygon.cxx
  TestPolyhedron0.cxx
  TestPolyhedron1.cxx
  TestPolyhedron3.cxx
  TestPolyhedronConvexity.cxx
  TestQuadraticPolygon.cxx
  TestRect.cxx
  TestSelectionSubtract.cxx
  TestSortFieldData.cxx
  TestTable.cxx
  TestTreeBFSIterator.cxx
  TestTreeDFSIterator.cxx
  TestTriangle.cxx
  TimePointLocators.cxx
  otherCellArray.cxx
  otherCellBoundaries.cxx
  otherCellPosition.cxx
  otherCellTypes.cxx
  otherColorTransferFunction.cxx
  otherEmptyCell.cxx
  otherFieldData.cxx
  otherImageData.cxx
  otherRectilinearGrid.cxx
  otherStructuredGrid.cxx
  otherUniformGrid.cxx
  quadCellConsistency.cxx
  quadraticEvaluation.cxx
  TestBoundingBox.cxx
  TestPlane.cxx
  TestStaticCellLinks.cxx
  TestStructuredData.cxx
  TestDataObjectTypes.cxx
  TestPolyDataRemoveDeletedCells.cxx
  UnitTestCells.cxx
  UnitTestImplicitDataSet.cxx
  UnitTestImplicitVolume.cxx
  UnitTestLine.cxx
  UnitTestPlanesIntersection.cxx
  UnitTestTriangleIntersection.cxx
  )
vtk_add_test_cxx(${vtk-module}CxxTests data_tests
  quadraticIntersection.cxx
  TestCellIterators.cxx,NO_VALID,NO_OUTPUT
  TestCellLocator.cxx,NO_DATA
  TestMeanValueCoordinatesInterpolation1.cxx
  TestMeanValueCoordinatesInterpolation2.cxx
  TestSmoothErrorMetric.cxx
  TestQuadraticPolygonFilters.cxx
  )
# add to the list but don't define a test
list(APPEND data_tests TestPolyhedron2.cxx)

vtk_add_test_cxx(${vtk-module}CxxTests output_tests
  TestKdTreeRepresentation.cxx,NO_DATA
  )
ExternalData_add_test(VTKData
  NAME TestPolyhedron2
  COMMAND ${vtk-module}CxxTests TestPolyhedron2
  -D DATA{../Data/polyhedron_mesh.vtu}
  )
set(all_tests
  ${tests}
  ${data_tests}
  ${output_tests}
  )
vtk_test_cxx_executable(${vtk-module}CxxTests all_tests)
