# set(TestCellGrid_ARGS
#   -D DATA{../Data/Input/dgHex2d.dg}
# )

vtk_add_test_python(
  ImplicitSum.py
#  LineIntersectQuadraticCells.py
  PerlinNoise.py
  SelectionLoop.py
  TestBoxPlaneIntersection.py
  TestCellGrid.py,NO_VALID
  TestComputeBounds.py,NO_DATA,NO_VALID
  TestCopyStructuredData.py,NO_DATA,NO_VALID
  TestGetBounds.py,NO_DATA,NO_VALID
  TestGetBounds2.py,NO_DATA,NO_VALID
  TestNumericArrayImageData.py,NO_DATA,NO_VALID
  TestPartitionedData.py,NO_DATA,NO_VALID
  TestPointInPolygon.py,NO_DATA,NO_VALID
  TestPolyDataGetCellPoints.py,NO_DATA,NO_VALID
  TestQuadricClustering.py
  TestStaticPointLocator.py,NO_DATA,NO_VALID
  TestStaticPointLocator2.py,NO_DATA,NO_VALID
  TestStaticPointLocator2D.py,NO_DATA,NO_VALID
  TestStaticPointLocator3.py,NO_DATA,NO_VALID
  TestStaticPointLocatorWithDataSet.py,NO_DATA,NO_VALID
  TestStarPolygon.py,NO_DATA,NO_VALID
  TestStructuredGrid.py
  TestTemplates.py,NO_DATA,NO_VALID
  headBone.py
  scalarColors.py
  scalarConn.py
  TestConvexPointSet.py
  otherDataSetAttributes.py,NO_DATA,NO_VALID
  quadricCut.py
  quadraticQuadDeriv.py,NO_VALID
  Test3DCellsEvaluatePosition.py,NO_DATA,NO_VALID,NO_OUTPUT
  TestICPTransform.py
  TestMolecule.py,NO_VALID
  TestClipPolyhedra.py
  TestClipPolyLine.py
  TestPolyhedronCutter.py,NO_DATA,NO_VALID
  TestHyperTreeGrid2DFull.py,NO_DATA,NO_VALID
  TestHyperTreeGrid2DFullGI.py,NO_DATA,NO_VALID
  TestHyperTreeGrid2DMask.py,NO_DATA,NO_VALID
  TestHyperTreeGrid2DWithFilters.py,NO_DATA,NO_VALID
  TestHyperTreeGrid3DFull.py,NO_DATA,NO_VALID
  TestHyperTreeGrid3DMask.py,NO_DATA,NO_VALID
  TestHyperTreeGrid3DWithFilters.py,NO_DATA,NO_VALID
  TestHyperTreeGrid3DMandel.py,NO_DATA,NO_VALID
  TestUniformHyperTreeGrid2DFull.py
  TestUniformHyperTreeGrid2DMask.py
  TestUniformHyperTreeGrid2DWithFilters.py,NO_DATA,NO_VALID
  TestUniformHyperTreeGrid3DWithFilters.py,NO_DATA,NO_VALID
  TestHyperTreeGrid3DCursorsMandel.py,NO_DATA,NO_VALID
  )
