create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
  TestClipHyperOctree.cxx
  TestHyperOctreeContourFilter.cxx
  TestHyperOctreeCutter.cxx
  TestHyperOctreeDual.cxx
  TestHyperOctreeSurfaceFilter.cxx
  TestHyperOctreeToUniformGrid.cxx
  TestHyperTreeGridTernary3DAxisCut.cxx
  TestHyperTreeGridTernary3DContour.cxx
  TestHyperTreeGridTernary3DGeometry.cxx
  TestHyperTreeGridTernary3DSphere.cxx
  TestHyperTreeGridTernary3DUnstructured.cxx
  TestHyperTreeGridTernary3DMaterialAxisCut.cxx
  TestHyperTreeGridTernary3DMaterialGeometry.cxx
  TestHyperTreeGridTernary3DMaterialUnstructured.cxx
  TestHyperTreeGridBinary2D.cxx

  EXTRA_INCLUDE vtkTestDriver.h
)

vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun ${vtk-module}CxxTests.cxx)

# Add all the executables
foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  if(VTK_DATA_ROOT)
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/Graphics/${TName}.png)
  else()
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach()
