SET(KIT Widgets)
# add tests that do not require data
SET(MyTests     
  BoxWidget.cxx
  BoxWidget2.cxx
#  TestAffineWidget.cxx
#  TestAngleWidget.cxx
#  TestBalloonWidget.cxx
#  TestBorderWidget.cxx
#  TestCameraWidget.cxx
#  TestCaptionWidget.cxx
#  TestCheckerboardWidget.cxx
  TestContourWidget2.cxx
#  TestDistanceWidget.cxx
  TestHandleWidget.cxx
#  TestHandleWidget2D.cxx
  TestImplicitPlaneWidget.cxx
  TestImplicitPlaneWidget2.cxx
#  TestLogoWidget.cxx
  TestOrientationMarkerWidget.cxx
#  TestPlaybackWidget.cxx
#  TestRectilinearWipeWidget.cxx
#  TestSeedWidget.cxx
#  TestSliderWidget.cxx
#  TestTextWidget.cxx
  TerrainPolylineEditor.cxx
  TestDijkstraGraphGeodesicPath.cxx
  TestParallelopipedWidget.cxx
  )
IF (VTK_DATA_ROOT)
  # add tests that require data
  SET(MyTests ${MyTests}
    ImagePlaneWidget.cxx
#    TestConstrainedHandleWidget.cxx
    TestSurfaceConstrainedHandleWidget.cxx
    TestPolygonalRepresentationHandleWidget.cxx
    TestSphereHandleWidget.cxx
#    TestFocalPlaneContour.cxx
#    TestOrientedGlyphContour.cxx
    TestImageTracerWidget.cxx
    TestLineWidget.cxx
#    TestLineWidget2.cxx
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

# remove some tests if there is no display
IF(NOT VTK_USE_DISPLAY)
  REMOVE(MyTests     
    BoxWidget.cxx
    BoxWidget2.cxx
    ImagePlaneWidget.cxx
    TestContourWidget2.cxx
    TestImageTracerWidget.cxx
    TestImplicitPlaneWidget.cxx
    TestLineWidget.cxx
    TestLogoWidgetAlphaBlending.cxx
    TestLogoWidgetDepthPeeling.cxx
    TestOrientationMarkerWidget.cxx
    TestOrthoPlanes.cxx
    TestParallelopipedWidget.cxx
    TestPlaneWidget.cxx
    TestPointWidget.cxx
    TestScalarBarWidget.cxx
    TestSplineWidget.cxx
    TestHandleWidget.cxx
    TerrainPolylineEditor.cxx
    )
ENDIF(NOT VTK_USE_DISPLAY)

CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx ${MyTests}
                       EXTRA_INCLUDE vtkTestDriver.h)

ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkWidgets)
SET (TestsToRun ${Tests})
REMOVE( TestsToRun 
   ${KIT}CxxTests.cxx 
   TestHandleWidget.cxx
   TerrainPolylineEditor.cxx
   TestDijkstraGraphGeodesicPath.cxx
   )

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

# Add any tests with special arguments here.. 
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
ENDIF(VTK_DATA_ROOT AND VTK_USE_DISPLAY)
