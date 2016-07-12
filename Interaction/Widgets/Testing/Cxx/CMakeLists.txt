set(TestOrientedGlyphContour_ARGS "--PlaybackFile" "DATA{../Data/Input/TestOrientedGlyphContourEventLog.txt}")
set(TerrainPolylineEditor_ARGS -ProjectionMode 2 -HeightOffset 100)
vtk_add_test_cxx(${vtk-module}CxxTests tests
  BoxWidget2.cxx
  BoxWidget.cxx
  ImagePlaneWidget.cxx
  TerrainPolylineEditor.cxx
  TestAffineWidget.cxx
  TestAngleWidget2D.cxx
  TestAngleWidget3D.cxx
  TestBalloonWidget.cxx
  TestBiDimensionalWidget.cxx
  TestBorderWidget.cxx
  TestBrokenLineWidget.cxx
  TestCameraWidget.cxx
  TestCaptionWidget.cxx
  TestCellCentersPointPlacer.cxx
  TestCenteredSliderWidget2D.cxx
  TestCheckerboardWidget.cxx
  TestConstrainedHandleWidget.cxx
  TestContourWidget2.cxx
  TestDijkstraImageGeodesicPath.cxx
  TestDistanceWidget3D.cxx
  TestDistanceWidget.cxx
  TestFinitePlaneWidget.cxx
  TestFixedSizeHandleRepresentation3D.cxx
  TestFocalPlaneContour.cxx
  TestHandleWidget2D.cxx
  TestHandleWidget.cxx
  TestImageActorContourWidget.cxx
  TestImageTracerWidget.cxx
  TestImplicitCylinderWidget.cxx
  TestImplicitCylinderWidget2.cxx
  TestImplicitPlaneWidget2b.cxx
  TestImplicitPlaneWidget2.cxx
  TestImplicitPlaneWidget3.cxx
  TestImplicitPlaneWidget2LockNormalToCamera.cxx
  TestImplicitPlaneWidget.cxx
  TestLogoWidgetAlphaBlending.cxx
  TestLogoWidget.cxx
  TestLogoWidgetDepthPeeling.cxx
  TestMultipleViewports.cxx
  TestOrientationMarkerWidget.cxx
  TestOrientationMarkerWidget2.cxx
  TestOrthoPlanes.cxx
  TestParallelopipedWidget.cxx
  TestPickingManagerSeedWidget.cxx
  TestPlaybackWidget.cxx
  TestPointHandleRepresentation3D.cxx
  TestProgrammaticPlacement.cxx
  TestProgressBarWidget.cxx
  TestProp3DButtonRepresentation.cxx,NO_VALID
  TestRectilinearWipeWidget.cxx
  TestResliceCursorWidget2.cxx
  TestResliceCursorWidget3.cxx
  TestScalarBarWidget.cxx
  TestSeedWidget2.cxx
  TestSeedWidget.cxx
  TestSeedWidgetNonUniformRepresentations.cxx
  TestSliderWidget2D.cxx
  TestSliderWidget.cxx
  TestSplineWidget.cxx
  TestSphereWidgetZoomInOut.cxx
  TestSphereWidget2CenterCursor.cxx
  TestTextWidget.cxx
  )
vtk_add_test_cxx(${vtk-module}CxxTests no_valid_tests
  NO_VALID
  vtkAngleRepresentation2DTest1.cxx
  vtkAngleRepresentation3DTest1.cxx
  vtkAngleWidgetTest1.cxx
  vtkBalloonRepresentationTest1.cxx
  vtkBalloonWidgetTest1.cxx
  vtkBiDimensionalRepresentation2DTest1.cxx
  vtkBiDimensionalWidgetTest1.cxx
  vtkBorderWidgetTest1.cxx
  vtkCaptionRepresentationTest1.cxx
  vtkCaptionWidgetTest1.cxx
  vtkConstrainedPointHandleRepresentationTest1.cxx
  vtkHoverWidgetTest1.cxx
  vtkLineRepresentationTest1.cxx
  vtkLineWidget2Test1.cxx
  vtkPolyLineRepresentationTest1.cxx
  vtkPolyLineWidgetTest1.cxx
  vtkSeedRepresentationTest1.cxx
  vtkSeedWidgetTest1.cxx
  vtkSplineRepresentationTest1.cxx
  vtkSplineWidget2Test1.cxx
  vtkTextRepresentationTest1.cxx
  vtkTextWidgetTest1.cxx
  )

# Tests with data
vtk_add_test_cxx(${vtk-module}CxxTests data_tests
  TestDijkstraGraphGeodesicPath.cxx
  TestLineWidget2.cxx
  TestLineWidget.cxx
  TestOrientedGlyphContour.cxx
  TestPlaneWidget.cxx
  TestPointWidget.cxx
  TestPolygonalHandleRepresentations.cxx
  TestPolygonalRepresentationHandleWidget.cxx
  TestPolygonalSurfaceConstrainedDistanceWidget.cxx
  TestPolyPlane.cxx,NO_VALID
  TestSphereHandleWidget.cxx
  TestSurfaceConstrainedHandleWidget.cxx
  TestTensorProbeWidget.cxx
  )

if(NOT VTK_OPENGL_HAS_OSMESA)
  vtk_add_test_cxx(${vtk-module}CxxTests ogl_tests
    TestPickingManagerWidgets.cxx
    )
endif()

# These tests depend on volume rendering classes not build for OpenGL2
if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  vtk_add_test_cxx(${vtk-module}CxxTests tests
    TestImageCroppingRegionsWidget.cxx
    )

  if(NOT VTK_LEGACY_REMOVE)
    vtk_add_test_cxx(${vtk-module}CxxTests tests
      TestButtonWidget.cxx
      )
  endif()
endif()

set(all_tests
  ${tests}
  ${no_valid_tests}
  ${data_tests}
  ${ogl_tests}
  )
vtk_test_cxx_executable(${vtk-module}CxxTests all_tests
  RENDERING_FACTORY
  )

# Allow local customization of timeout for this test.
if(NOT VTK_TEST_TIMEOUT_TerrainPolylineEditor)
  set(VTK_TEST_TIMEOUT_TerrainPolylineEditor 90)
endif()
set_tests_properties(${vtk-module}Cxx-TerrainPolylineEditor
  PROPERTIES TIMEOUT ${VTK_TEST_TIMEOUT_TerrainPolylineEditor})

if(NOT VTK_OPENGL_HAS_OSMESA)
  # Allow local customization of timeout for this test.
  if(NOT VTK_TEST_TIMEOUT_TestPickingManagerSeedWidget)
    set(VTK_TEST_TIMEOUT_TestPickingManagerSeedWidget 60)
  endif()
  set_tests_properties(${vtk-module}Cxx-TestPickingManagerSeedWidget
    PROPERTIES TIMEOUT ${VTK_TEST_TIMEOUT_TestPickingManagerSeedWidget})
endif()

# On some machines this test takes ~220 seconds to complete, allow for some
# extra time
if(NOT VTK_TEST_TIMEOUT_TestSphereHandleWidget)
  set(VTK_TEST_TIMEOUT_TestSphereHandleWidget 300)
endif()
set_tests_properties(${vtk-module}Cxx-TestSphereHandleWidget
    PROPERTIES TIMEOUT ${VTK_TEST_TIMEOUT_TestSphereHandleWidget}
               RUN_SERIAL ON)
