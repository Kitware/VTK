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
  vtkAbstractContextBufferId.cxx
  vtkAbstractContextBufferId.h
  vtkAxis.cxx
  vtkAxis.h
  vtkBarMark.cxx
  vtkBarMark.h
  vtkBlockItem.cxx
  vtkBlockItem.h
  vtkBrush.cxx
  vtkBrush.h
  vtkChart.cxx
  vtkChart.h
  vtkChartLegend.cxx
  vtkChartLegend.h
  vtkChartParallelCoordinates.cxx
  vtkChartParallelCoordinates.h
  vtkChartXY.cxx
  vtkChartXY.h
  vtkColorSeries.cxx
  vtkColorSeries.h
  vtkContext2D.cxx
  vtkContext2D.h
  vtkContextActor.cxx
  vtkContextActor.h
  vtkContextBufferId.cxx
  vtkContextBufferId.h
  vtkContextDevice2D.cxx
  vtkContextDevice2D.h
  vtkContextItem.cxx
  vtkContextItem.h
  vtkContextMapper2D.cxx
  vtkContextMapper2D.h
  vtkContextScene.cxx
  vtkContextScene.h
  vtkContextView.cxx
  vtkContextView.h
  vtkDataElement.cxx
  vtkDataElement.h
  vtkDataValue.cxx
  vtkDataValue.h
  vtkImageItem.cxx
  vtkImageItem.h
  vtkLineMark.cxx
  vtkLineMark.h
  vtkMark.cxx
  vtkMark.h
  vtkMarkUtil.cxx
  vtkMarkUtil.h
  vtkOpenGLContextBufferId.cxx
  vtkOpenGLContextBufferId.h
  vtkOpenGLContextDevice2D.cxx
  vtkOpenGLContextDevice2D.h
  vtkPanelMark.cxx
  vtkPanelMark.h
  vtkPen.cxx
  vtkPen.h
  vtkPlot.cxx
  vtkPlot.h
  vtkPlotBar.cxx
  vtkPlotBar.h
  vtkPlotGrid.cxx
  vtkPlotGrid.h
  vtkPlotLine.cxx
  vtkPlotLine.h
  vtkPlotParallelCoordinates.cxx
  vtkPlotParallelCoordinates.h
  vtkPlotPoints.cxx
  vtkPlotPoints.h
  vtkTooltipItem.cxx
  vtkTooltipItem.h
  vtkWedgeMark.cxx
  vtkWedgeMark.h
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkAbstractContextBufferId
  vtkChart
  vtkContextDevice2D
  vtkContextItem
  vtkContextMapper2D
  vtkDataValue
  vtkPlot
  ABSTRACT
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkAbstractContextBufferId
  vtkContextBufferId
  vtkBarMark
  vtkChartColors
  vtkDataElement
  vtkDataValue
  vtkLineMark
  vtkMark
  vtkMarkUtil
  vtkOpenGLContextBufferId
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

# Headers without a corresponding source file that should be installed.
# Seems like we should improve the kit macro to cover this at some point.
IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  SET(Kit_HDRS
    vtkColor.h
    vtkVector.h
    )
  INSTALL(FILES ${Kit_HDRS}
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}
    COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

#-----------------------------------------------------------------------------
# Include CMake code common to all kits.
INCLUDE(${VTK_CMAKE_DIR}/KitCommonBlock.cmake)
#-----------------------------------------------------------------------------

#TARGET_LINK_LIBRARIES(vtkCharts ${QT_LIBRARIES})

