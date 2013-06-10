set(TerrainPolylineEditor_ARGS -ProjectionMode 2 -HeightOffset 100)
vtk_add_test_cxx(
  BoxWidget.cxx
  BoxWidget2.cxx
  # ImagePlaneWidget.cxx # todo (unsatisfied deps)
  TerrainPolylineEditor.cxx
  TestAffineWidget.cxx
  TestAngleWidget2D.cxx
  TestAngleWidget3D.cxx
  TestBalloonWidget.cxx
  TestBiDimensionalWidget.cxx
  TestBorderWidget.cxx
  # TestBrokenLineWidget.cxx # todo (unsatisfied deps)
  # TestButtonWidget.cxx # todo (unsatistfied deps)
  TestCameraWidget.cxx
  TestCellCentersPointPlacer.cxx
  TestCenteredSliderWidget2D.cxx
  TestCheckerboardWidget.cxx
  TestConstrainedHandleWidget.cxx
  TestContourWidget2.cxx
  TestDijkstraImageGeodesicPath.cxx
  TestDistanceWidget.cxx
  TestDistanceWidget3D.cxx
  TestFixedSizeHandleRepresentation3D.cxx
  TestFocalPlaneContour.cxx
  TestHandleWidget.cxx
  TestHandleWidget2D.cxx
  # TestImageActorContourWidget.cxx # todo(unsatistfied deps)
  TestImageTracerWidget.cxx
  TestImplicitPlaneWidget.cxx
  TestImplicitPlaneWidget2.cxx
  TestImplicitPlaneWidget2LockNormalToCamera.cxx
  TestImplicitPlaneWidget2b.cxx
  TestLogoWidget.cxx
  TestLogoWidgetAlphaBlending.cxx
  TestLogoWidgetDepthPeeling.cxx
  TestOrientationMarkerWidget.cxx
  # TestOrthoPlanes.cxx # todo (unsatistfied deps)
  TestParallelopipedWidget.cxx
  TestPickingManagerSeedWidget.cxx
  TestPlaybackWidget.cxx
  TestPointHandleRepresentation3D.cxx
  TestProgrammaticPlacement.cxx
  TestProp3DButtonRepresentation.cxx,NO_VALID
  TestRectilinearWipeWidget.cxx
  # TestResliceCursorWidget2.cxx # todo (unsatistfied deps)
  # TestResliceCursorWidget3.cxx # todo (unsatistfied deps)
  TestScalarBarWidget.cxx
  TestSeedWidget.cxx
  TestSeedWidget2.cxx
  TestSeedWidgetNonUniformRepresentations.cxx
  TestSliderWidget.cxx
  TestSliderWidget2D.cxx
  # TestSplineWidget.cxx # todo (unsatistfied deps)
  TestTextWidget.cxx
  )
vtk_add_test_cxx(NO_VALID
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
  vtkSeedRepresentationTest1.cxx
  vtkSeedWidgetTest1.cxx
  vtkSplineWidget2Test1.cxx
  vtkTextRepresentationTest1.cxx
  vtkTextWidgetTest1.cxx
  )

# Tests with data
vtk_add_test_cxx(
  TestDijkstraGraphGeodesicPath.cxx
  TestLineWidget.cxx
  TestLineWidget2.cxx
  TestPlaneWidget.cxx
  TestPointWidget.cxx
  TestPolyPlane.cxx,NO_VALID
  TestPolygonalHandleRepresentations.cxx
  TestPolygonalRepresentationHandleWidget.cxx
  TestPolygonalSurfaceConstrainedDistanceWidget.cxx
  TestSphereHandleWidget.cxx
  TestSurfaceConstrainedHandleWidget.cxx
  TestTensorProbeWidget.cxx
  )

if(NOT VTK_OPENGL_HAS_OSMESA)
  vtk_add_test_cxx(TestPickingManagerWidgets.cxx)
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)

set_tests_properties(${vtk-module}Cxx-TerrainPolylineEditor
  PROPERTIES TIMEOUT 90)

if(NOT VTK_OPENGL_HAS_OSMESA)
  set_tests_properties(${vtk-module}Cxx-TestPickingManagerSeedWidget
    PROPERTIES TIMEOUT 60)
endif()
