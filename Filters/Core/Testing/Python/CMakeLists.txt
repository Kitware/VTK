# Data files:
#  deciPlane,NO_RT
#  financialField,NO_RT
#  skinOrder,NO_RT
#  MassProperties,NO_RT
#  financialField2,NO_RT
#  financialField3,NO_RT

vtk_add_test_python(
  CellDataToPointData.py
  Delaunay2D.py
  Delaunay2DAlpha.py
  Delaunay2DTransform.py
  Delaunay3D.py
  Delaunay3DAlphaTest.py
  QuadricDecimation.py
  QuadricDecimation2.py
  StreamPolyData.py
  TestElevationFilter.py
  TestFlyingEdges2D.py
  TestFlyingEdges3D.py
  TestFlyingEdges3DWithInterpolation.py
  TestFlyingEdgesExtents.py
  TestFlyingEdgesPlaneCutter.py
  TestFlyingEdgesPlaneCutterInterpolation.py
  TestGridSynchronizedTemplates3D.py
  TestMarchingSquares.py
  TestProbeFilterImageInput.py
  TestRectilinearSynchronizedTemplates.py
  TestScalarTrees.py
  TestSphereTreeFilter.py
  TestSphereTreeFilter2.py
  TestStructuredGridPlaneCutter.py
  TestSynchronizedTemplates2D.py
  TestSynchronizedTemplates3D.py
  TestContourGrid.py,NO_VALID
  TestContourPolyData.py,NO_VALID
  TestTensorGlyph.py,NO_RT
  TestTextureGlyph.py
  TestTriangleFilter.py
  TestUnstructuredGridPlaneCutter.py
  TestVectorDotNorm.py
  capCow.py
  capSphere.py
  clipArt.py
  combStreamers.py
  constrainedDelaunay.py
  contourCells.py
  contourQuadraticCells.py
  contourQuadraticTetra.py,NO_RT
  createBFont.py
  cutLoop.py
  cutPolygons.py
  cutter.py
  dispPlot.py
  fieldToPolyData.py
  fieldToRGrid.py
  fieldToSGrid.py
  fieldToUGrid.py
  glyphComb.py
  hull.py
  mergeFilter.py
  multipleComponentContour.py
  multipleIso.py
  polyConn.py
  probe.py
  probeComb.py
  reverseNormals.py
  smoothMeshOnMesh.py
  streamComb.py
  streamSurface.py
  streamSurface2.py
  stripF.py
  teapotHulls.py
  tubeComb.py
  deciFranFace.py,NO_RT
  TestContourCases.py,NO_DATA,NO_VALID
  pointsPrecisions.py,NO_OUTPUT,NO_DATA,NO_VALID,NO_RT
  TestCompositeCutter.py,NO_VALID
  testPDNormals.py,NO_DATA,NO_VALID
  )
