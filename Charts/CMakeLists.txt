SET(KIT Charts)
SET(UKIT CHARTS)

SET(KIT_TCL_LIBS vtkViewsTCL ${VTK_TK_LIBRARIES})
SET(KIT_PYTHON_LIBS vtkViewsPythonD)
SET(KIT_JAVA_LIBS vtkViewsJava)

IF (JAVA_AWT_LIBRARY)
  SET(KIT_JAVA_LIBS ${KIT_JAVA_LIBS} ${JAVA_AWT_LIBRARY})
ENDIF (JAVA_AWT_LIBRARY)

SET(KIT_INTERFACE_LIBRARIES vtkViews)
SET(KIT_LIBS vtkftgl)

SET(Kit_SRCS
  vtkAbstractContextBufferId.cxx
  vtkAbstractContextItem.cxx
  vtkAxis.cxx
  vtkAxisExtended.cxx
  vtkBlockItem.cxx
  vtkBrush.cxx
  vtkChart.cxx
  vtkChartLegend.cxx
  vtkChartHistogram2D.cxx
  vtkChartMatrix.cxx
  vtkChartParallelCoordinates.cxx
  vtkChartXY.cxx
  vtkChartPie.cxx
  vtkColorLegend.cxx
  vtkPlotPie.cxx
  vtkColorSeries.cxx
  vtkColorTransferFunctionItem.cxx
  vtkColorTransferControlPointsItem.cxx
  vtkCompositeControlPointsItem.cxx
  vtkCompositeTransferFunctionItem.cxx
  vtkContext2D.cxx
  vtkContextActor.cxx
  vtkContextBufferId.cxx
  vtkContextClip.cxx
  vtkContextDevice2D.cxx
  vtkContextInteractorStyle.cxx
  vtkContextItem.cxx
  vtkContextKeyEvent.cxx
  vtkContextMapper2D.cxx
  vtkContextMouseEvent.cxx
  vtkContextScene.cxx
  vtkContextTransform.cxx
  vtkContextView.cxx
  vtkControlPointsItem.cxx
  vtkImageItem.cxx
  vtkLookupTableItem.cxx
  vtkOpenGLContextBufferId.cxx
  vtkOpenGL2ContextDevice2D.cxx
  vtkOpenGLContextDevice2D.cxx
  vtkPen.cxx
  vtkPiecewiseControlPointsItem.cxx
  vtkPiecewiseFunctionItem.cxx
  vtkPiecewisePointHandleItem.cxx
  vtkPlot.cxx
  vtkPlotBar.cxx
  vtkPlotGrid.cxx
  vtkPlotHistogram2D.cxx
  vtkPlotLine.cxx
  vtkPlotStacked.cxx
  vtkPlotParallelCoordinates.cxx
  vtkPlotPoints.cxx
  vtkScalarsToColorsItem.cxx
  vtkScatterPlotMatrix.cxx
  vtkTooltipItem.cxx
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkAbstractContextBufferId
  vtkAbstractContextItem
  vtkChart
  vtkContextDevice2D
  vtkContextItem
  vtkContextMapper2D
  vtkControlPointsItem
  vtkPlot
  vtkScalarsToColorsItem
  ABSTRACT
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkAbstractContextBufferId
  vtkContextBufferId
  vtkContextKeyEvent
  vtkContextMouseEvent
  vtkChartColors
  vtkOpenGLContextBufferId
  vtkOpenGL2ContextDevice2D
  vtkOpenGLContextDevice2D
  WRAP_EXCLUDE
)

# Need to be able to include QApplication if Qt is used for runtime checks
IF(VTK_USE_QT)
  SET(QT_DONT_USE_QTGUI)
  INCLUDE(${QT_USE_FILE})
  IF (APPLE)
    LIST (APPEND KIT_INTERFACE_LIBRARIES ${QT_LIBRARIES})
  ELSE (APPLE)
    LIST (APPEND KIT_LIBS ${QT_LIBRARIES})
  ENDIF (APPLE)
ENDIF(VTK_USE_QT)

#-----------------------------------------------------------------------------
# Include CMake code common to all kits.
INCLUDE(${VTK_CMAKE_DIR}/KitCommonBlock.cmake)
#-----------------------------------------------------------------------------
