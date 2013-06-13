set(MyTests
  TestGraphWeightEuclideanDistanceFilter.cxx
  BoxClipPolyData.cxx
  BoxClipTetrahedra.cxx
  BoxClipTriangulate.cxx
  BoxClipTriangulateAndInterpolate.cxx
  TestBooleanOperationPolyDataFilter.cxx
  TestBooleanOperationPolyDataFilter2.cxx
  TestContourTriangulatorCutter.cxx
  TestDeformPointSet.cxx
  TestDensifyPolyData.cxx
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
  set(MyTests
    ${MyTests}
    TestContourTriangulator.cxx
    TestContourTriangulatorMarching.cxx
    TestGradientAndVorticity.cxx
    TestIconGlyphFilterGravity.cxx
    TestQuadraturePoints.cxx
    TestYoungsMaterialInterface.cxx
    )
endif()

# Set the tolerance higher for a few tests that need it
set(TestDensifyPolyDataError 15)

# Use the testing object factory, to reduce boilerplate code in tests.
include(vtkTestingObjectFactory)

vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun CxxTests.cxx)

# Add all the executables
foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  if(VTK_DATA_ROOT)
    if(${${TName}Error})
      set(_error_threshold ${${TName}Error})
    else()
      set(_error_threshold 10)
    endif()
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/Graphics/${TName}.png
        -E ${_error_threshold})
 else()
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName})
 endif()
endforeach()
