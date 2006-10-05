# if we have rendering add the following tests
IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
  SET(KIT Graphics)
  # add tests that do not require data
  SET(MyTests     
    Mace.cxx
    expCos.cxx
    BoxClipTriangulate.cxx
    CellLocator.cxx
    PointLocator.cxx
    FrustumClip.cxx
    RGrid.cxx
    TestClipHyperOctree.cxx
    TestExtractSelection.cxx
    TestHyperOctreeContourFilter.cxx
    TestHyperOctreeCutter.cxx
    TestHyperOctreeDual.cxx
    TestHyperOctreeSurfaceFilter.cxx
    TestHyperOctreeToUniformGrid.cxx
    TestSortDataArray.cxx
    )
  IF (VTK_DATA_ROOT)
    # add tests that require data
    SET(MyTests ${MyTests}
      BoxClipPolyData.cxx
      BoxClipTetrahedra.cxx
      BoxClipTriangulateAndInterpolate.cxx
      MeshQuality.cxx
      TestHierarchicalBoxPipeline.cxx
      TestMultiBlock.cxx
      TestUnstructuredGridGeometryFilter.cxx
      UnstructuredGridGradients.cxx
      UnstructuredGridCellGradients.cxx
      )
  ENDIF (VTK_DATA_ROOT)
  CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx ${MyTests}
                         EXTRA_INCLUDE vtkTestDriver.h)
  ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
  TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkRendering vtkIO)
  SET (TestsToRun ${Tests})
  REMOVE (TestsToRun ${KIT}CxxTests.cxx)

  #
  # Add all the executables 
  FOREACH (test ${TestsToRun})
    GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
    IF (VTK_DATA_ROOT)
      ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}
          -D ${VTK_DATA_ROOT}
          -T ${VTK_BINARY_DIR}/Testing/Temporary
          -V Baseline/${KIT}/${TName}.png)
    ELSE (VTK_DATA_ROOT)
      ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}) 
    ENDIF (VTK_DATA_ROOT)
  ENDFOREACH (test) 
ENDIF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
