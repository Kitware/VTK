vtk_add_test_cxx(Graphics
  TestGraphWeightEuclideanDistanceFilter.cxx,NO_VALID
  BoxClipPolyData.cxx
  BoxClipTetrahedra.cxx
  BoxClipTriangulate.cxx,NO_VALID
  BoxClipTriangulateAndInterpolate.cxx
  TestBooleanOperationPolyDataFilter.cxx
  TestBooleanOperationPolyDataFilter2.cxx
  TestContourTriangulator.cxx
  TestContourTriangulatorCutter.cxx
  TestContourTriangulatorMarching.cxx
  TestDeformPointSet.cxx
  TestDensifyPolyData.cxx,-E15
  TestDistancePolyDataFilter.cxx
  TestImageDataToPointSet.cxx,NO_VALID
  TestIntersectionPolyDataFilter.cxx
  TestIntersectionPolyDataFilter2.cxx,NO_VALID
  TestRectilinearGridToPointSet.cxx,NO_VALID
  TestReflectionFilter.cxx,NO_VALID
  TestUncertaintyTubeFilter.cxx
  )

# Tests with data
if(VTK_DATA_ROOT)
  vtk_add_test_cxx(Graphics
    TestGradientAndVorticity.cxx,NO_VALID
    TestIconGlyphFilterGravity.cxx
    TestQuadraturePoints.cxx
    TestYoungsMaterialInterface.cxx
    )
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)
