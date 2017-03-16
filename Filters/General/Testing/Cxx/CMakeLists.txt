set(TestDensifyPolyData_ARGS -E 15)
set(TestDataSetGradient_ARGS -E 25)
set(TestDataSetGradientPrecompute_ARGS -E 25)

vtk_add_test_cxx(${vtk-module}CxxTests tests
  ArrayExtractFactoredArray.cxx,NO_VALID
  ArrayMatricizeArray.cxx,NO_VALID
  ArrayNormalizeMatrixVectors.cxx,NO_VALID
  CellTreeLocator.cxx,NO_VALID
  TestPassArrays.cxx,NO_VALID
  TestPassThrough.cxx,NO_VALID
  TestTessellator.cxx,NO_VALID
  expCos.cxx
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
  TestCountFaces.cxx,NO_VALID
  TestCountVertices.cxx,NO_VALID
  TestDeformPointSet.cxx
  TestDensifyPolyData.cxx
  TestDistancePolyDataFilter.cxx
  TestGraphWeightEuclideanDistanceFilter.cxx,NO_VALID
  TestImageDataToPointSet.cxx,NO_VALID
  TestIntersectionPolyDataFilter3.cxx
  TestIntersectionPolyDataFilter2.cxx,NO_VALID
  TestIntersectionPolyDataFilter.cxx
  TestRectilinearGridToPointSet.cxx,NO_VALID
  TestReflectionFilter.cxx,NO_VALID
  TestSplitByCellScalarFilter.cxx,NO_VALID
  TestTableSplitColumnComponents.cxx,NO_VALID
  TestTransformFilter.cxx,NO_VALID
  TestTransformPolyDataFilter.cxx,NO_VALID
  TestUncertaintyTubeFilter.cxx
  UnitTestMultiThreshold.cxx,NO_VALID
  )
# Tests with data
vtk_add_test_cxx(${vtk-module}CxxTests data_tests
  BoxClipPointData.cxx
  BoxClipOrientedPointData.cxx
  TestDataSetGradient.cxx
  TestDataSetGradientPrecompute.cxx
  TestGradientAndVorticity.cxx,NO_VALID
  TestIconGlyphFilterGravity.cxx
  TestQuadraturePoints.cxx
  TestYoungsMaterialInterface.cxx
  )

set(all_tests
  ${tests}
  ${data_tests}
  )

vtk_test_cxx_executable(${vtk-module}CxxTests all_tests
  RENDERING_FACTORY)

# On some machines this test takes some extra time
if(NOT VTK_TEST_TIMEOUT_TestYoungsMaterialInterface)
  set(VTK_TEST_TIMEOUT_TestYoungsMaterialInterface 300)
endif()
set_tests_properties(${vtk-module}Cxx-TestYoungsMaterialInterface
  PROPERTIES TIMEOUT ${VTK_TEST_TIMEOUT_TestYoungsMaterialInterface})
