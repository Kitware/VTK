set(MyTests
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
  TestDijkstraGraphGeodesicPath.cxx
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
  TestLineWidget.cxx
  TestLineWidget2.cxx
  TestLogoWidget.cxx
  TestLogoWidgetAlphaBlending.cxx
  TestLogoWidgetDepthPeeling.cxx
  TestOrientationMarkerWidget.cxx
  # TestOrthoPlanes.cxx # todo (unsatistfied deps)
  TestParallelopipedWidget.cxx
  TestPickingManagerWidgets.cxx
  TestPickingManagerSeedWidget.cxx
  TestPlaneWidget.cxx
  TestPlaybackWidget.cxx
  TestPointHandleRepresentation3D.cxx
  TestPointWidget.cxx
  TestPolyPlane.cxx
  TestPolygonalHandleRepresentations.cxx
  TestPolygonalRepresentationHandleWidget.cxx
  TestPolygonalSurfaceConstrainedDistanceWidget.cxx
  TestProgrammaticPlacement.cxx
  TestProp3DButtonRepresentation.cxx
  TestRectilinearWipeWidget.cxx
  # TestResliceCursorWidget2.cxx # todo (unsatistfied deps)
  # TestResliceCursorWidget3.cxx # todo (unsatistfied deps)
  TestScalarBarWidget.cxx
  TestSeedWidget.cxx
  TestSeedWidget2.cxx
  TestSeedWidgetNonUniformRepresentations.cxx
  TestSliderWidget.cxx
  TestSliderWidget2D.cxx
  TestSphereHandleWidget.cxx
  # TestSplineWidget.cxx # todo (unsatistfied deps)
  TestSurfaceConstrainedHandleWidget.cxx
  TestTensorProbeWidget.cxx
  TestTextWidget.cxx
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

# Use the testing object factory, to reduce boilerplate code in tests.
include("${vtkTestingRendering_SOURCE_DIR}/vtkTestingObjectFactory.cmake")

vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun CxxTests.cxx)
list(REMOVE_ITEM TestsToRun TerrainPolylineEditor.cxx)

if(VTK_DATA_ROOT)
  add_test(NAME ${vtk-module}Cxx-TerrainPolylineEditor
    COMMAND ${vtk-module}CxxTests TerrainPolylineEditor
      -ProjectionMode 2
      -HeightOffset 100
      -D ${VTK_DATA_ROOT}
      -T ${VTK_TEST_OUTPUT_DIR}
      -V Baseline/Widgets/TerrainPolylineEditor.png)
endif()

# Add all the executables
foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  if(VTK_DATA_ROOT)
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/Widgets/${TName}.png)
  else()
    add_test(NAME ${vtk-module}Cxx-${TName} COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach()
