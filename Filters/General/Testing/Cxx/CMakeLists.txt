vtk_add_test_cxx(Graphics
  TestGraphWeightEuclideanDistanceFilter.cxx
  BoxClipPolyData.cxx
  BoxClipTetrahedra.cxx
  BoxClipTriangulate.cxx
  BoxClipTriangulateAndInterpolate.cxx
  TestBooleanOperationPolyDataFilter.cxx
  TestBooleanOperationPolyDataFilter2.cxx
  TestContourTriangulator.cxx
  TestContourTriangulatorCutter.cxx
  TestContourTriangulatorMarching.cxx
  TestDeformPointSet.cxx
  TestDensifyPolyData.cxx,-E15
  TestDistancePolyDataFilter.cxx
  TestImageDataToPointSet.cxx
  TestIntersectionPolyDataFilter.cxx
  TestIntersectionPolyDataFilter2.cxx
  TestRectilinearGridToPointSet.cxx
  TestReflectionFilter.cxx
  TestUncertaintyTubeFilter.cxx
  )

# Tests with data
if(VTK_DATA_ROOT)
  vtk_add_test_cxx(Graphics
    TestGradientAndVorticity.cxx
    TestIconGlyphFilterGravity.cxx
    TestQuadraturePoints.cxx
    TestYoungsMaterialInterface.cxx
    )
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)
