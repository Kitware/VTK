vtk_add_test_cxx(
  BoxClipPolyData.cxx
  BoxClipTetrahedra.cxx
  BoxClipTriangulateAndInterpolate.cxx
  BoxClipTriangulate.cxx,NO_VALID
  TestAppendPoints.cxx,NO_VALID
  TestBooleanOperationPolyDataFilter2.cxx
  TestBooleanOperationPolyDataFilter.cxx
  TestContourTriangulatorCutter.cxx
  TestContourTriangulator.cxx
  TestContourTriangulatorMarching.cxx
  TestDeformPointSet.cxx
  TestDensifyPolyData.cxx,-E15
  TestDistancePolyDataFilter.cxx
  TestGraphWeightEuclideanDistanceFilter.cxx,NO_VALID
  TestImageDataToPointSet.cxx,NO_VALID
  TestIntersectionPolyDataFilter2.cxx,NO_VALID
  TestIntersectionPolyDataFilter.cxx
  TestRectilinearGridToPointSet.cxx,NO_VALID
  TestReflectionFilter.cxx,NO_VALID
  TestTableSplitColumnComponents.cxx,NO_VALID
  TestTransformFilter.cxx,NO_VALID
  TestTransformPolyDataFilter.cxx,NO_VALID
  TestUncertaintyTubeFilter.cxx
  )

# Tests with data
vtk_add_test_cxx(
  TestGradientAndVorticity.cxx,NO_VALID
  TestIconGlyphFilterGravity.cxx
  TestQuadraturePoints.cxx
  TestYoungsMaterialInterface.cxx
  )

vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)
