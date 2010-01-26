SET(KIT Widgets)


# Most (all?) Widgets tests require access to the display:
#
IF(VTK_USE_DISPLAY)


# add tests that do not require data
SET(MyTests     
  BoxWidget.cxx
  BoxWidget2.cxx
#  TestAffineWidget.cxx
  TestAngleWidget2D.cxx
  TestAngleWidget3D.cxx
  TestBalloonWidget.cxx
#  TestBorderWidget.cxx
#  TestCameraWidget.cxx
  TestCaptionWidget.cxx
  TestCheckerboardWidget.cxx
  TestContourWidget2.cxx
  TestDistanceWidget.cxx
  TestHandleWidget.cxx
#  TestHandleWidget2D.cxx
  TestImplicitPlaneWidget.cxx
  TestImplicitPlaneWidget2.cxx
  TestImplicitPlaneWidget2b.cxx
#  TestLogoWidget.cxx
  TestOrientationMarkerWidget.cxx
#  TestPlaybackWidget.cxx
  TestRectilinearWipeWidget.cxx
  TestSeedWidget.cxx
  TestSliderWidget.cxx
  TestSliderWidget2D.cxx
  TestTextWidget.cxx
  TerrainPolylineEditor.cxx
  TestDijkstraGraphGeodesicPath.cxx
  TestParallelopipedWidget.cxx
  TestSeedWidgetNonUniformRepresentations.cxx
  )

IF (VTK_DATA_ROOT)
  # add tests that require data
  SET(MyTests ${MyTests}
    ImagePlaneWidget.cxx
    TestBiDimensionalWidget.cxx
    TestDijkstraImageGeodesicPath.cxx
    TestImageActorContourWidget.cxx
#    TestConstrainedHandleWidget.cxx
    TestSurfaceConstrainedHandleWidget.cxx
    TestPolygonalRepresentationHandleWidget.cxx
    TestPolygonalHandleRepresentations.cxx
    TestSphereHandleWidget.cxx
#    TestFocalPlaneContour.cxx
#    TestOrientedGlyphContour.cxx
    TestImageTracerWidget.cxx
    TestLineWidget.cxx
    TestLineWidget2.cxx
    TestLogoWidgetAlphaBlending.cxx
    TestLogoWidgetDepthPeeling.cxx
    TestOrthoPlanes.cxx
    TestPlaneWidget.cxx
    TestPointWidget.cxx
    TestScalarBarWidget.cxx
#    TestSeedWidget2.cxx
    TestSplineWidget.cxx
    TestTensorProbeWidget.cxx
    )
ENDIF (VTK_DATA_ROOT)


INCLUDE(${VTK_SOURCE_DIR}/Rendering/vtkTestingObjectFactory.cmake)

ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkWidgets)
SET (TestsToRun ${Tests})
REMOVE( TestsToRun
   ${KIT}CxxTests.cxx
   TestHandleWidget.cxx
   TerrainPolylineEditor.cxx
   TestDijkstraGraphGeodesicPath.cxx
   TestRectilinearWipeWidget.cxx
   )

# Add all the tests
#
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

# Add any tests with special arguments here.. 
#
IF(VTK_DATA_ROOT AND VTK_USE_DISPLAY)
  ADD_TEST( TerrainPolylineEditor ${CXX_TEST_PATH}/${KIT}CxxTests 
         TerrainPolylineEditor 
         -ProjectionMode 2
         -HeightOffset 100
         -T ${VTK_BINARY_DIR}/Testing/Temporary
         -D ${VTK_DATA_ROOT}
         -V Baseline/${KIT}/TerrainPolylineEditor.png)
  ADD_TEST( TestHandleWidget ${CXX_TEST_PATH}/${KIT}CxxTests 
         TestHandleWidget
         -ConstrainHandlesToPlane
         -T ${VTK_BINARY_DIR}/Testing/Temporary
         -D ${VTK_DATA_ROOT}
         -V Baseline/${KIT}/TestHandleWidget.png)
  ADD_TEST( TestDijkstraGraphGeodesicPath ${CXX_TEST_PATH}/${KIT}CxxTests 
         TestDijkstraGraphGeodesicPath
         -DistanceOffset 20.0
         -T ${VTK_BINARY_DIR}/Testing/Temporary
         -D ${VTK_DATA_ROOT}
         -V Baseline/${KIT}/TestDijkstraGraphGeodesicPath.png)

  ADD_TEST( TestRectilinearWipeWidget0 ${CXX_TEST_PATH}/${KIT}CxxTests 
         TestRectilinearWipeWidget
         0
         -T ${VTK_BINARY_DIR}/Testing/Temporary
         -D ${VTK_DATA_ROOT}
         -V Baseline/${KIT}/TestRectilinearWipeWidget0.png)
  ADD_TEST( TestRectilinearWipeWidget1 ${CXX_TEST_PATH}/${KIT}CxxTests 
         TestRectilinearWipeWidget
         1
         -T ${VTK_BINARY_DIR}/Testing/Temporary
         -D ${VTK_DATA_ROOT}
         -V Baseline/${KIT}/TestRectilinearWipeWidget1.png)
  ADD_TEST( TestRectilinearWipeWidget2 ${CXX_TEST_PATH}/${KIT}CxxTests 
         TestRectilinearWipeWidget
         2
         -T ${VTK_BINARY_DIR}/Testing/Temporary
         -D ${VTK_DATA_ROOT}
         -V Baseline/${KIT}/TestRectilinearWipeWidget2.png)
  ADD_TEST( TestRectilinearWipeWidget3 ${CXX_TEST_PATH}/${KIT}CxxTests 
         TestRectilinearWipeWidget
         3
         -T ${VTK_BINARY_DIR}/Testing/Temporary
         -D ${VTK_DATA_ROOT}
         -V Baseline/${KIT}/TestRectilinearWipeWidget3.png)
  ADD_TEST( TestRectilinearWipeWidget4 ${CXX_TEST_PATH}/${KIT}CxxTests 
         TestRectilinearWipeWidget
         4
         -T ${VTK_BINARY_DIR}/Testing/Temporary
         -D ${VTK_DATA_ROOT}
         -V Baseline/${KIT}/TestRectilinearWipeWidget4.png)
  ADD_TEST( TestRectilinearWipeWidget5 ${CXX_TEST_PATH}/${KIT}CxxTests 
         TestRectilinearWipeWidget
         5
         -T ${VTK_BINARY_DIR}/Testing/Temporary
         -D ${VTK_DATA_ROOT}
         -V Baseline/${KIT}/TestRectilinearWipeWidget5.png)
  ADD_TEST( TestRectilinearWipeWidget6 ${CXX_TEST_PATH}/${KIT}CxxTests 
         TestRectilinearWipeWidget
         6
         -T ${VTK_BINARY_DIR}/Testing/Temporary
         -D ${VTK_DATA_ROOT}
         -V Baseline/${KIT}/TestRectilinearWipeWidget6.png)
ENDIF(VTK_DATA_ROOT AND VTK_USE_DISPLAY)


ENDIF(VTK_USE_DISPLAY)
