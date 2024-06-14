set(TestDensifyPolyData_ARGS -E 15)
set(TestDataSetGradient_ARGS -E 25)
set(TestDataSetGradientPrecompute_ARGS -E 25)

vtk_add_test_cxx(vtkFiltersGeneralCxxTests tests
  ArrayExtractFactoredArray.cxx,NO_VALID
  ArrayMatricizeArray.cxx,NO_VALID
  ArrayNormalizeMatrixVectors.cxx,NO_VALID
  BoxClipPolyData.cxx
  BoxClipTetrahedra.cxx
  BoxClipTriangulate.cxx,NO_VALID
  BoxClipTriangulateAndInterpolate.cxx
  TestAppendLocationAttributes.cxx,NO_VALID
  TestAppendPoints.cxx,NO_VALID
  TestBlockIdScalars.cxx,NO_VALID
  TestBooleanOperationPolyDataFilter.cxx
  TestBooleanOperationPolyDataFilter2.cxx
  TestCellValidator.cxx,NO_VALID
  TestCellValidatorFilter.cxx,NO_VALID
  TestCleanUnstructuredGridStrategies.cxx,NO_VALID
  TestContourTriangulator.cxx
  TestContourTriangulatorBadData.cxx
  TestContourTriangulatorCutter.cxx
  TestContourTriangulatorHoles.cxx
  TestContourTriangulatorMarching.cxx
  TestCountFaces.cxx,NO_VALID
  TestCountVertices.cxx,NO_VALID
  TestDeflectNormals.cxx
  TestDeformPointSet.cxx
  TestDensifyPolyData.cxx
  TestDistancePolyDataFilter.cxx
  TestDistancePolyDataFilter2.cxx
  TestExtractGhostCells.cxx,NO_VALID
  TestGraphWeightEuclideanDistanceFilter.cxx,NO_VALID
  TestGroupDataSetsFilter.cxx,NO_VALID
  TestHOStableClip.cxx,NO_VALID
  TestImageDataToPointSet.cxx,NO_VALID
  TestIntersectionPolyDataFilter.cxx
  TestIntersectionPolyDataFilter2.cxx,NO_VALID
  TestIntersectionPolyDataFilter3.cxx
  TestIntersectionPolyDataFilter4.cxx,NO_VALID
  TestJoinTables.cxx,NO_VALID
  TestLoopBooleanPolyDataFilter.cxx
  TestMergeArrays.cxx,NO_VALID
  TestMergeCells.cxx,NO_VALID
  TestMergeTimeFilter.cxx,NO_VALID
  TestMergeVectorComponents.cxx,NO_VALID
  TestOverlappingAMRLevelIdScalars.cxx,NO_VALID
  TestPassArrays.cxx,NO_VALID
  TestPassSelectedArrays.cxx,NO_VALID
  TestPassThrough.cxx,NO_VALID
  TestRandomAttributeGeneratorHTG.cxx,NO_VALID,NO_OUTPUT
  TestRectilinearGridToPointSet.cxx,NO_VALID
  TestReflectionFilter.cxx,NO_VALID
  TestSpatioTemporalHarmonicsAttribute.cxx
  TestSplitByCellScalarFilter.cxx,NO_VALID
  TestTableFFT.cxx,NO_VALID
  TestTableSplitColumnComponents.cxx,NO_VALID
  TestTemporalPathLineFilter.cxx,NO_VALID
  TestTessellator.cxx,NO_VALID
  TestTransformFilter.cxx,NO_VALID
  TestTransformPolyDataFilter.cxx,NO_VALID
  TestUncertaintyTubeFilter.cxx
  TestWarpScalarGenerateEnclosure.cxx
  UnitTestMultiThreshold.cxx,NO_VALID
  expCos.cxx
  )
# Tests with data
vtk_add_test_cxx(vtkFiltersGeneralCxxTests data_tests
  BoxClipOrientedPointData.cxx
  BoxClipPointData.cxx
  TestClipClosedSurface1.cxx
  TestClipClosedSurface2.cxx
  TestDataSetGradient.cxx
  TestDataSetGradientPrecompute.cxx
  TestDateToNumeric.cxx
  TestGradientAndVorticity.cxx,NO_VALID
  TestIconGlyphFilterGravity.cxx
  TestQuadraturePoints.cxx
  TestYoungsMaterialInterface.cxx
  )

if (TARGET VTK::IOIOSS)
  vtk_add_test_cxx(vtkFiltersGeneralCxxTests data_tests
    TestAnimateModes.cxx
    )
endif()

set(all_tests
  ${tests}
  ${data_tests}
  )

vtk_test_cxx_executable(vtkFiltersGeneralCxxTests all_tests
  DISABLE_FLOATING_POINT_EXCEPTIONS
  RENDERING_FACTORY
  )

# On some machines this test takes some extra time
if(NOT VTK_TEST_TIMEOUT_TestYoungsMaterialInterface)
  set(VTK_TEST_TIMEOUT_TestYoungsMaterialInterface 300)
endif()
set_tests_properties(VTK::FiltersGeneralCxx-TestYoungsMaterialInterface
  PROPERTIES TIMEOUT ${VTK_TEST_TIMEOUT_TestYoungsMaterialInterface})
