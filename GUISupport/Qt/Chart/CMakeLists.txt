PROJECT(vtkQtChart)

# Set up Qt if it hasn't been done already.
IF(NOT QT4_FOUND)
  SET(QT_REQUIRED TRUE)
  SET(QT_MIN_VERSION "4.2.2")
  FIND_PACKAGE(Qt4)
ENDIF(NOT QT4_FOUND)

IF(NOT QT4_FOUND)
  MESSAGE(SEND_ERROR "Qt ${QT_MIN_VERSION} or greater not found.  Please check the QT_QMAKE_EXECUTABLE variable.")
ELSE(NOT QT4_FOUND)

  INCLUDE(${QT_USE_FILE})

  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

  SET(vtkQtChart_SOURCES
    vtkQtBarChart.cxx
    vtkQtBarChartOptions.cxx
    vtkQtBarChartSeriesOptions.cxx
    vtkQtChartArea.cxx
    vtkQtChartAxis.cxx
    vtkQtChartAxisCornerDomain.cxx
    vtkQtChartAxisDomain.cxx
    vtkQtChartAxisDomainPriority.cxx
    vtkQtChartAxisLayer.cxx
    vtkQtChartAxisModel.cxx
    vtkQtChartAxisOptions.cxx
    vtkQtChartColorPaletteGenerator.cxx
    vtkQtChartContentsArea.cxx
    vtkQtChartContentsSpace.cxx
    vtkQtChartGridLayer.cxx
    vtkQtChartHelpFormatter.cxx
    vtkQtChartInteractor.cxx
    vtkQtChartInteractorSetup.cxx
    vtkQtChartLayer.cxx
    vtkQtChartLayerDomain.cxx
    vtkQtChartLegend.cxx
    vtkQtChartLegendModel.cxx
    vtkQtChartMouseBox.cxx
    vtkQtChartMouseFunction.cxx
    vtkQtChartMousePan.cxx
    vtkQtChartMouseSelection.cxx
    vtkQtChartMouseSelectionHandler.cxx
    vtkQtChartMouseZoom.cxx
    vtkQtChartPointLocator.cxx
    vtkQtChartScene.cxx
    vtkQtChartSeriesDomain.cxx
    vtkQtChartSeriesDomainGroup.cxx
    vtkQtChartSeriesLayer.cxx
    vtkQtChartSeriesModel.cxx
    vtkQtChartSeriesModelCollection.cxx
    vtkQtChartSeriesModelRange.cxx
    vtkQtChartSeriesOptions.cxx
    vtkQtChartSeriesSelection.cxx
    vtkQtChartSeriesSelectionHandler.cxx
    vtkQtChartSeriesSelectionModel.cxx
    vtkQtChartStyleGenerator.cxx
    vtkQtChartStyleManager.cxx
    vtkQtChartTableSeriesModel.cxx
    vtkQtChartTitle.cxx
    vtkQtChartWidget.cxx
    vtkQtChartZoomHistory.cxx
    vtkQtLineChart.cxx
    vtkQtLineChartOptions.cxx
    vtkQtLineChartSeriesOptions.cxx
    vtkQtPointMarker.cxx
    vtkQtPolylineItem.cxx
    vtkQtSimplePointLocator.cxx
    vtkQtStackedChart.cxx
    vtkQtStackedChartOptions.cxx
    vtkQtStackedChartSeriesOptions.cxx
    vtkQtStatisticalBoxChart.cxx
    vtkQtStatisticalBoxChartOptions.cxx
    vtkQtStatisticalBoxChartSeriesOptions.cxx
  )
  
  SET(vtkQtChart_MOCHEADERS
    vtkQtBarChart.h
    vtkQtBarChartOptions.h
    vtkQtBarChartSeriesOptions.h
    vtkQtChartArea.h
    vtkQtChartAxis.h
    vtkQtChartAxisLayer.h
    vtkQtChartAxisModel.h
    vtkQtChartAxisOptions.h
    vtkQtChartColorPaletteGenerator.h
    vtkQtChartStyleGenerator.h
    vtkQtChartContentsSpace.h
    vtkQtChartGridLayer.h
    vtkQtChartInteractor.h
    vtkQtChartLayer.h
    vtkQtChartLegend.h
    vtkQtChartLegendModel.h
    vtkQtChartMouseFunction.h
    vtkQtChartMouseSelection.h
    vtkQtChartSeriesLayer.h
    vtkQtChartSeriesModel.h
    vtkQtChartSeriesModelCollection.h
    vtkQtChartSeriesModelRange.h
    vtkQtChartSeriesOptions.h
    vtkQtChartSeriesSelectionModel.h
    vtkQtChartTableSeriesModel.h
    vtkQtChartTitle.h
    vtkQtChartWidget.h
    vtkQtLineChart.h
    vtkQtLineChartOptions.h
    vtkQtLineChartSeriesOptions.h
    vtkQtStackedChart.h
    vtkQtStackedChartOptions.h
    vtkQtStackedChartSeriesOptions.h
    vtkQtStatisticalBoxChart.h
    vtkQtStatisticalBoxChartOptions.h
    vtkQtStatisticalBoxChartSeriesOptions.h
    )
  SET(vtkQtChart_HEADERS
    vtkQtChartGraphicsItemTypes.h
    )

SET(MOC_INCLUDE_DIRS ${MOC_INCLUDE_DIRS} ${QT_INCLUDE_DIR})

# Reduce the number of dirs that get included on moc command line
# since it causes issues on Windows 2000.
GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${MOC_INCLUDE_DIRS}")

  QT4_WRAP_CPP(vtkQtChart_MOC_SOURCES ${vtkQtChart_MOCHEADERS})

SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

  SOURCE_GROUP("Generated" FILES
    ${vtkQtChart_MOC_SOURCES}
  )

  ADD_LIBRARY(vtkQtChart
    ${vtkQtChart_SOURCES}
    ${vtkQtChart_MOCHEADERS}
    ${vtkQtChart_HEADERS}
    ${vtkQtChart_MOC_SOURCES}
  )

  # Apply user-defined properties to the library target.
  IF(VTK_LIBRARY_PROPERTIES)
    SET_TARGET_PROPERTIES(vtkQtChart PROPERTIES ${VTK_LIBRARY_PROPERTIES})
  ENDIF(VTK_LIBRARY_PROPERTIES)

  TARGET_LINK_LIBRARIES(vtkQtChart
    ${QT_LIBRARIES}
  )

  SET(VTKQTCHART_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
  CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/vtkQtChartExport.h.in"
                 "${CMAKE_CURRENT_BINARY_DIR}/vtkQtChartExport.h")

  IF(NOT VTK_INSTALL_NO_DEVELOPMENT AND VTK_INSTALL_INCLUDE_DIR_CM24)
    INSTALL(FILES ${vtkQtChart_MOCHEADERS} ${vtkQtChart_HEADERS}
      DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24} COMPONENT Development)
  ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT AND VTK_INSTALL_INCLUDE_DIR_CM24)

  IF(NOT VTK_INSTALL_NO_LIBRARIES AND VTK_INSTALL_BIN_DIR_CM24 AND VTK_INSTALL_LIB_DIR_CM24)
    INSTALL(TARGETS vtkQtChart
      RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT RuntimeLibraries
      LIBRARY DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT Development)
  ENDIF(NOT VTK_INSTALL_NO_LIBRARIES AND VTK_INSTALL_BIN_DIR_CM24 AND VTK_INSTALL_LIB_DIR_CM24)

ENDIF(NOT QT4_FOUND)

