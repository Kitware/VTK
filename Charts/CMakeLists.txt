SET(KIT Charts)
SET(UKIT CHARTS)

SET(KIT_TCL_LIBS vtkHybridTCL vtkViewsTCL ${VTK_TK_LIBRARIES})
SET(KIT_PYTHON_LIBS vtkHybridPythonD vtkViewsPythonD)
SET(KIT_JAVA_LIBS vtkHybridJava vtkViewsJava)

IF (JAVA_AWT_LIBRARY)
  SET(KIT_JAVA_LIBS ${KIT_JAVA_LIBS} ${JAVA_AWT_LIBRARY})
ENDIF (JAVA_AWT_LIBRARY)

SET(KIT_INTERFACE_LIBRARIES vtkHybrid vtkViews)
SET(KIT_LIBS vtkIO vtkftgl
  ${VTK_FREETYPE_LIBRARIES}
)

SET(Kit_SRCS
  vtkAxis.cxx
  vtkBlockItem.cxx
  vtkBrush.cxx
  vtkChart.cxx
  vtkChartXY.cxx
  vtkContext2D.cxx
  vtkContextActor.cxx
  vtkContextDevice2D.cxx
  vtkContextItem.cxx
  vtkContextMapper2D.cxx
  vtkContextScene.cxx
  vtkContextView.cxx
  vtkImageItem.cxx
  vtkOpenGLContextDevice2D.cxx
  vtkPen.cxx
  vtkPlot.cxx
  vtkPlotGrid.cxx
  vtkPlotLine.cxx
  vtkPlotPoints.cxx
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkChart
  vtkContextDevice2D
  vtkContextItem
  vtkContextMapper2D
  vtkPlot
  ABSTRACT
  )

#-----------------------------------------------------------------------------
# Include CMake code common to all kits.
INCLUDE(${VTK_CMAKE_DIR}/KitCommonBlock.cmake)
#-----------------------------------------------------------------------------

#TARGET_LINK_LIBRARIES(vtkCharts ${QT_LIBRARIES})

