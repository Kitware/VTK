SET(KIT Hybrid)
# add tests that do not require data
SET(MyTests
  TestBSplineWarp.cxx
  TestImageStencilData.cxx
  X3DTest.cxx
  )
IF (VTK_DATA_ROOT)
  # add tests that require data
  SET(MyTests ${MyTests}
#    TestAxisActor3D.cxx
    TestCubeAxes2DMode.cxx
    TestCubeAxes3.cxx
    TestCubeAxesWithGridLines.cxx
    TestCubeAxesWithXLines.cxx
    TestCubeAxesWithYLines.cxx
    TestCubeAxesWithZLines.cxx
    TestCubeAxesWithXInnerGrids.cxx
    TestCubeAxesWithYInnerGrids.cxx
    TestCubeAxesWithZInnerGrids.cxx
    TestCubeAxesWithXInnerPolys.cxx
    TestCubeAxesWithYInnerPolys.cxx
    TestCubeAxesWithZInnerPolys.cxx
    TestBarChartActor.cxx
    TestExodusImplicitArrays.cxx
    #TestLabelPlacerExodus.cxx
    TestLegendScaleActor.cxx
    TestPolarAxes.cxx
    TestPolyDataSilhouette.cxx
    TestPieChartActor.cxx
    TestSpiderPlotActor.cxx
    TestVRMLNormals.cxx
    TestTDxGeo.cxx
    TestLegendBoxActor.cxx
    )
  # Temporarily disable LIC tests on Mac platforms
  IF (VTK_USE_GLSL_SHADERS AND NOT APPLE)
    SET(MyTests ${MyTests}
      TestImageDataLIC2D.cxx
      TestStructuredGridLIC2DXSlice.cxx
      TestStructuredGridLIC2DYSlice.cxx
      TestStructuredGridLIC2DZSlice.cxx
      )
  ENDIF ()
ENDIF (VTK_DATA_ROOT)

# remove some tests if there is no display of if they need args
IF(NOT VTK_USE_DISPLAY)
  list(REMOVE_ITEM MyTests
    TestAxisActor3D.cxx
    TestBarChartActor.cxx
    TestLegendScaleActor.cxx
    TestPieChartActor.cxx
    TestPolyDataSilhouette.cxx
    TestSpiderPlotActor.cxx
    X3DTest.cxx
    TestLegendBoxActor.cxx
    )
ENDIF(NOT VTK_USE_DISPLAY)

IF (VTK_DATA_ROOT)
  ADD_TEST(TestPolyDataSilhouette ${CXX_TEST_PATH}/${KIT}CxxTests
           TestPolyDataSilhouette ${VTK_DATA_ROOT}/Data/cow.vtp)
ENDIF (VTK_DATA_ROOT)

INCLUDE(${VTK_SOURCE_DIR}/Rendering/vtkTestingObjectFactory.cmake)

ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkHybrid vtkRendering vtkImaging vtkIO )
SET (TestsToRun ${Tests})
LIST( REMOVE_ITEM TestsToRun ${KIT}CxxTests.cxx TestPolyDataSilhouette.cxx TestImageStencilData.cxx)

#
# Add all the executables
FOREACH (test ${TestsToRun})
  GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
  IF (VTK_DATA_ROOT)
    ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -V Baseline/${KIT}/${TName}.png)
  ELSE (VTK_DATA_ROOT)
    ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName})
  ENDIF (VTK_DATA_ROOT)
ENDFOREACH (test)

IF(VTK_DATA_ROOT AND VTK_USE_DISPLAY)
  ADD_TEST(TestAddStencilData ${CXX_TEST_PATH}/${KIT}CxxTests
         TestImageStencilData 1
        -T ${VTK_BINARY_DIR}/Testing/Temporary
        -D ${VTK_DATA_ROOT}
        -V Baseline/${KIT}/TestAddStencilData.png)
  ADD_TEST(TestSubtractStencilData ${CXX_TEST_PATH}/${KIT}CxxTests
         TestImageStencilData 2
        -D ${VTK_DATA_ROOT}
        -V Baseline/${KIT}/TestSubtractStencilData.png)
  ADD_TEST(TestClipStencilData ${CXX_TEST_PATH}/${KIT}CxxTests
         TestImageStencilData 3
        -D ${VTK_DATA_ROOT}
        -V Baseline/${KIT}/TestClipStencilData.png)
ENDIF(VTK_DATA_ROOT AND VTK_USE_DISPLAY)
