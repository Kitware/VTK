create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
  TestClipHyperOctree.cxx
  TestHyperOctreeContourFilter.cxx
  TestHyperOctreeCutter.cxx
  TestHyperOctreeDual.cxx
  TestHyperOctreeSurfaceFilter.cxx
  TestHyperOctreeToUniformGrid.cxx
  TestHyperTreeGridBinary2D.cxx
  TestHyperTreeGridBinary2DIJK.cxx
  TestHyperTreeGridBinary2DMaterial.cxx
  TestHyperTreeGridBinary2DMaterialIJK.cxx
  TestHyperTreeGridBinaryEllipseMaterial.cxx
  TestHyperTreeGridBinaryHyperbolicParaboloidMaterial.cxx
  TestHyperTreeGridTernary2D.cxx
  TestHyperTreeGridTernary2DMaterial.cxx
  TestHyperTreeGridTernary2DMaterialBits.cxx
  TestHyperTreeGridTernary2DFullMaterialBits.cxx
  TestHyperTreeGridTernary2DBiMaterial.cxx
  TestHyperTreeGridTernary3DAxisCut.cxx
  TestHyperTreeGridTernary3DAxisCutMaterial.cxx
  TestHyperTreeGridTernary3DContour.cxx
  TestHyperTreeGridTernary3DContourMaterial.cxx
  TestHyperTreeGridTernary3DClip.cxx
  TestHyperTreeGridTernary3DCut.cxx
  TestHyperTreeGridTernary3DGeometry.cxx
  TestHyperTreeGridTernary3DGeometryMaterial.cxx
  TestHyperTreeGridTernary3DGeometryMaterialBits.cxx
  TestHyperTreeGridTernary3DGeometryLargeMaterialBits.cxx
  TestHyperTreeGridTernary3DUnstructured.cxx
  TestHyperTreeGridTernary3DUnstructuredMaterial.cxx
  TestHyperTreeGridTernaryHyperbola.cxx
  TestHyperTreeGridTernarySphereMaterial.cxx
  EXTRA_INCLUDE vtkTestDriver.h
)

vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun ${vtk-module}CxxTests.cxx)

# Add all the executables
foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  if(VTK_DATA_ROOT)
    string(REGEX REPLACE "Bits" "" TNameNoBits ${TName})
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/Graphics/${TNameNoBits}.png)
  else()
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach()
