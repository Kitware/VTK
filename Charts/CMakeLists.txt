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
  vtkBarMark.cxx
  vtkBlockItem.cxx
  vtkBrush.cxx
  vtkChart.cxx
  vtkChartXY.cxx
  vtkColorSeries.cxx
  vtkContext2D.cxx
  vtkContextActor.cxx
  vtkContextDevice2D.cxx
  vtkContextItem.cxx
  vtkContextMapper2D.cxx
  vtkContextScene.cxx
  vtkContextView.cxx
  vtkDataElement.cxx
  vtkDataValue.cxx
  vtkImageItem.cxx
  vtkLineMark.cxx
  vtkMark.cxx
  vtkMarkUtil.cxx
  vtkOpenGLContextDevice2D.cxx
  vtkPanelMark.cxx
  vtkPen.cxx
  vtkPlot.cxx
  vtkPlotGrid.cxx
  vtkPlotLine.cxx
  vtkPlotPoints.cxx
  vtkWedgeMark.cxx
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkChart
  vtkContextDevice2D
  vtkContextItem
  vtkContextMapper2D
  vtkDataValue
  vtkPlot
  ABSTRACT
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkBarMark
  vtkChartColors
  vtkDataElement
  vtkDataValue
  vtkLineMark
  vtkMark
  vtkMarkUtil
  vtkPanelMark
  vtkValueHolder
  vtkWedgeMark
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

#TARGET_LINK_LIBRARIES(vtkCharts ${QT_LIBRARIES})

