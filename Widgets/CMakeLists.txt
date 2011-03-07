SET(KIT Widgets)
SET(UKIT WIDGETS)

SET(KIT_TCL_LIBS vtkRenderingTCL vtkHybridTCL)
SET(KIT_PYTHON_LIBS vtkRenderingPythonD vtkHybridPythonD)
SET(KIT_JAVA_LIBS vtkRenderingJava vtkHybridJava)
SET(KIT_INTERFACE_LIBRARIES vtkRendering vtkHybrid vtkVolumeRendering)
SET(KIT_LIBS ${OPENGL_gl_LIBRARY})

IF (VTK_OPENGL_HAS_OSMESA AND OSMESA_LIBRARY)
  SET (KIT_LIBS ${KIT_LIBS} ${OSMESA_LIBRARY} )
ENDIF (VTK_OPENGL_HAS_OSMESA AND OSMESA_LIBRARY)

SET ( Kit_SRCS
vtk3DWidget.cxx
vtkAbstractWidget.cxx
vtkAffineRepresentation.cxx
vtkAffineRepresentation2D.cxx
vtkAffineWidget.cxx
vtkAngleRepresentation.cxx
vtkAngleRepresentation2D.cxx
vtkAngleRepresentation3D.cxx
vtkAngleWidget.cxx
vtkAxesTransformWidget.cxx
vtkAxesTransformRepresentation.cxx
vtkBalloonRepresentation.cxx
vtkBalloonWidget.cxx
vtkBezierContourLineInterpolator.cxx
vtkBiDimensionalRepresentation.cxx
vtkBiDimensionalRepresentation2D.cxx
vtkBiDimensionalWidget.cxx
vtkBorderRepresentation.cxx
vtkBorderWidget.cxx
vtkBoundedPlanePointPlacer.cxx
vtkBoxRepresentation.cxx
vtkBoxWidget.cxx
vtkBoxWidget2.cxx
vtkButtonRepresentation.cxx
vtkButtonWidget.cxx
vtkCameraRepresentation.cxx
vtkCameraWidget.cxx
vtkCaptionRepresentation.cxx
vtkCaptionWidget.cxx
vtkCenteredSliderRepresentation.cxx
vtkCenteredSliderWidget.cxx
vtkContinuousValueWidget.cxx
vtkContinuousValueWidgetRepresentation.cxx
vtkContourLineInterpolator.cxx
vtkContourRepresentation.cxx
vtkFocalPlaneContourRepresentation.cxx
vtkContourWidget.cxx
vtkCheckerboardRepresentation.cxx
vtkCheckerboardWidget.cxx
vtkConstrainedPointHandleRepresentation.cxx
vtkDijkstraImageContourLineInterpolator.cxx
vtkDistanceWidget.cxx
vtkDistanceRepresentation.cxx
vtkDistanceRepresentation2D.cxx
vtkDistanceRepresentation3D.cxx
vtkEllipsoidTensorProbeRepresentation.cxx
vtkEvent.cxx
vtkFocalPlanePointPlacer.cxx
vtkHandleRepresentation.cxx
vtkHandleWidget.cxx
vtkHoverWidget.cxx
vtkImageActorPointPlacer.cxx
vtkImageOrthoPlanes.cxx
vtkImagePlaneWidget.cxx
vtkImageTracerWidget.cxx
vtkImplicitPlaneRepresentation.cxx
vtkImplicitPlaneWidget.cxx
vtkImplicitPlaneWidget2.cxx
vtkLinearContourLineInterpolator.cxx
vtkLineRepresentation.cxx
vtkLineWidget.cxx
vtkLineWidget2.cxx
vtkLogoRepresentation.cxx
vtkLogoWidget.cxx
vtkOrientationMarkerWidget.cxx
vtkOrientedGlyphContourRepresentation.cxx
vtkOrientedGlyphFocalPlaneContourRepresentation.cxx
vtkParallelopipedRepresentation.cxx
vtkParallelopipedWidget.cxx
vtkClosedSurfacePointPlacer.cxx
vtkPlaneWidget.cxx
vtkPlaybackRepresentation.cxx
vtkPlaybackWidget.cxx
vtkPointHandleRepresentation2D.cxx
vtkPointHandleRepresentation3D.cxx
vtkPointPlacer.cxx
vtkPointWidget.cxx
vtkPolyDataSourceWidget.cxx
vtkPolyDataPointPlacer.cxx
vtkAbstractPolygonalHandleRepresentation3D.cxx
vtkPolygonalHandleRepresentation3D.cxx
vtkOrientedPolygonalHandleRepresentation3D.cxx
vtkPolygonalSurfacePointPlacer.cxx
vtkPolyDataContourLineInterpolator.cxx
vtkPolygonalSurfaceContourLineInterpolator.cxx
vtkProp3DButtonRepresentation.cxx
vtkRectilinearWipeRepresentation.cxx
vtkRectilinearWipeWidget.cxx
vtkScalarBarRepresentation.cxx
vtkScalarBarWidget.cxx
vtkSeedRepresentation.cxx
vtkSeedWidget.cxx
vtkSliderRepresentation.cxx
vtkSliderRepresentation2D.cxx
vtkSliderRepresentation3D.cxx
vtkSliderWidget.cxx
vtkSphereHandleRepresentation.cxx
vtkSphereRepresentation.cxx
vtkSphereWidget.cxx
vtkSphereWidget2.cxx
vtkSplineRepresentation.cxx
vtkSplineWidget.cxx
vtkSplineWidget2.cxx
vtkTensorProbeWidget.cxx
vtkTensorProbeRepresentation.cxx
vtkTerrainDataPointPlacer.cxx
vtkTerrainContourLineInterpolator.cxx
vtkTextRepresentation.cxx
vtkTexturedButtonRepresentation.cxx
vtkTexturedButtonRepresentation2D.cxx
vtkTextWidget.cxx
vtkWidgetCallbackMapper.cxx
vtkWidgetEvent.cxx
vtkWidgetEventTranslator.cxx
vtkWidgetRepresentation.cxx
vtkWidgetSet.cxx
vtkXYPlotWidget.cxx
)

SET_SOURCE_FILES_PROPERTIES(
vtk3DWidget
vtkAbstractWidget
vtkAbstractPolygonalHandleRepresentation3D
vtkAffineRepresentation
vtkAngleRepresentation
vtkBiDimensionalRepresentation
vtkButtonRepresentation
vtkContinuousValueWidget
vtkContinuousValueWidgetRepresentation
vtkContourLineInterpolator
vtkContourRepresentation
vtkFocalPlaneContourRepresentation
vtkDistanceRepresentation
vtkHandleRepresentation
vtkPolyDataContourLineInterpolator
vtkPolyDataSourceWidget
vtkSliderRepresentation
vtkTensorProbeRepresentation
vtkWidgetRepresentation
ABSTRACT
)

SET(Kit_EXTRA_SRCS)
SET(Kit_EXTRA_CMDS)
SET(Kit_TCL_EXTRA_SRCS)
SET(Kit_PYTHON_EXTRA_SRCS)
SET(Kit_JAVA_EXTRA_SRCS)
SET(KIT_TCL_DEPS)
SET(KIT_PYTHON_DEPS)
SET(KIT_JAVA_DEPS)

#-----------------------------------------------------------------------------
# Include CMake code common to all kits.
INCLUDE(${VTK_CMAKE_DIR}/KitCommonBlock.cmake)
#-----------------------------------------------------------------------------
