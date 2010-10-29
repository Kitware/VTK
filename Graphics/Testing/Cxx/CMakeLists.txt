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
    TestAppendSelection.cxx
#    TestAppendPolyData.cxx #pending a bug fix
    TestAssignAttribute.cxx
    TestBSPTree.cxx
    TestCellDataToPointData.cxx
    TestDensifyPolyData.cxx
    TestClipHyperOctree.cxx
    TestConvertSelection.cxx
    TestDelaunay2D.cxx
    TestExtraction.cxx
    TestExtractSelection.cxx
    TestHyperOctreeContourFilter.cxx
    TestHyperOctreeCutter.cxx
    TestHyperOctreeDual.cxx
    TestHyperOctreeSurfaceFilter.cxx
    TestHyperOctreeToUniformGrid.cxx
    TestNamedComponents.cxx
    TestMeanValueCoordinatesInterpolation1.cxx
    TestMeanValueCoordinatesInterpolation2.cxx
    TestPolyDataPointSampler.cxx
    TestPolyhedron0.cxx
    TestPolyhedron1.cxx
    TestSelectEnclosedPoints.cxx
    TestTessellatedBoxSource.cxx
    TestTessellator.cxx
    TestUncertaintyTubeFilter.cxx
    TestDecimatePolylineFilter.cxx
    )

  IF(VTK_USE_BOOST)
    SET(MyTests
      ${MyTests}
      TestReebGraph.cxx
      )
  ENDIF(VTK_USE_BOOST)

  # Add Matlab Engine and Matlab Mex related tests.
  IF(VTK_USE_MATLAB_MEX)
    INCLUDE(${MATLAB_MEX_USE_FILE})
    SET(MyTests ${MyTests}
      TestMatlabEngineInterface.cxx
      TestMatlabEngineFilter.cxx
    )
  ENDIF(VTK_USE_MATLAB_MEX)

  # Add Gnu R interface related files.
  IF(VTK_USE_GNU_R)
    SET(MyTests ${MyTests}
      TestRRandomTableSource.cxx
      TestRCalculatorFilter.cxx
      TestRInterface.cxx
      )
  ENDIF(VTK_USE_GNU_R)

  IF (VTK_DATA_ROOT)
    # add tests that require data
    SET(MyTests ${MyTests}
      BoxClipPolyData.cxx
      BoxClipTetrahedra.cxx
      BoxClipTriangulateAndInterpolate.cxx
      MeshQuality.cxx
      TestActor2DTextures.cxx
      TestArrayCalculator.cxx
      TestGradientAndVorticity.cxx
      TestHierarchicalBoxPipeline.cxx
      TestIconGlyphFilterGravity.cxx
      TestIncrementalOctreePointLocator.cxx
      TestMultiBlock.cxx
      TestQuadraturePoints.cxx
      TestUnstructuredGridGeometryFilter.cxx
      UnstructuredGridGradients.cxx
      UnstructuredGridCellGradients.cxx
      UnstructuredGridFastGradients.cxx
      )
    IF (VTK_USE_PARALLEL)
      SET(MyTests ${MyTests}
        TemporalStatistics.cxx
        )
    ENDIF (VTK_USE_PARALLEL)
  ENDIF (VTK_DATA_ROOT)
  CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx ${MyTests}
                         EXTRA_INCLUDE vtkTestDriver.h)
  ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
  TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkRendering vtkIO)
  IF (VTK_USE_PARALLEL)
    TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkParallel ${OPENGL_gl_LIBRARY})
  ENDIF (VTK_USE_PARALLEL)
  IF (VTK_USE_GNU_R OR VTK_USE_MATLAB_MEX)
    TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkInfovis)
  ENDIF (VTK_USE_GNU_R OR VTK_USE_MATLAB_MEX)
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
