# set up sources to build
SET ( QVTKLibSrcs
    vtkEventQtSlotConnect.cxx
    vtkQtConnection.cxx
    QVTKWidget.cxx
    )

SET ( QVTKMocHeaders
    QVTKWidget.h 
    vtkQtConnection.h
    )

SET ( QVTKNonMocHeaders
    ${CMAKE_CURRENT_SOURCE_DIR}/QVTKWin32Header.h
    ${CMAKE_CURRENT_SOURCE_DIR}/vtkEventQtSlotConnect.h
    )

INCLUDE_DIRECTORIES ( ${CMAKE_CURRENT_BINARY_DIR} )

# add files that require Qt 4
SET ( QVTKLibSrcs
    ${QVTKLibSrcs}
    QFilterTreeProxyModel.cxx
    QVTKPaintEngine.cxx
    QVTKPaintEngine.h
    vtkQtAbstractModelAdapter.cxx
    vtkQtAnnotationLayersModelAdapter.cxx
    vtkQtSQLDatabase.cxx
    vtkQtSQLQuery.cxx
    vtkQtTableModelAdapter.cxx
    vtkQtTimePointUtility.cxx
    vtkQtTreeModelAdapter.cxx 
    )
    
SET ( QVTKMocHeaders
    ${QVTKMocHeaders}
    QFilterTreeProxyModel.h
    vtkQtAbstractModelAdapter.h
    vtkQtAnnotationLayersModelAdapter.h
    vtkQtTableModelAdapter.h
    vtkQtTreeModelAdapter.h
    )

# add additional files depending on infovis and/or views
IF(VTK_USE_VIEWS)
  SET( QVTKLibSrcs ${QVTKLibSrcs}
    vtkQtAnnotationView.cxx
    vtkQtBarChartView.cxx
    vtkQtChartRepresentation.cxx
    vtkQtChartView.cxx
    vtkQtLineChartView.cxx
    vtkQtListView.cxx
    vtkQtRecordView.cxx
    vtkQtStackedChartView.cxx
    vtkQtStatisticalBoxChartView.cxx
    vtkQtTableRepresentation.cxx
    vtkQtTableView.cxx
    vtkQtTreeView.cxx
    vtkQtView.cxx
    ) 
  SET_SOURCE_FILES_PROPERTIES(vtkQtView.cxx ABSTRACT )
  SET ( QVTKMocHeaders ${QVTKMocHeaders}
    vtkQtAnnotationView.h
    vtkQtBarChartView.h
    vtkQtChartView.h
    vtkQtLineChartView.h
    vtkQtListView.h
    vtkQtRecordView.h
    vtkQtStackedChartView.h
    vtkQtStatisticalBoxChartView.h
    vtkQtTableView.h
    vtkQtTreeView.h
    vtkQtView.h
    )
  SET ( QVTKNonMocHeaders ${QVTKNonMocHeaders}
    vtkQtChartRepresentation.h
    vtkQtTableRepresentation.h
    )

  # Rich-text view requires Qt >= 4.5.0
  # Rich-text depends on QT Webkit which is not portable on Unix (AIX & HP-UX)
  IF(QT_QTWEBKIT_FOUND)
    OPTION(VTK_QT_USE_WEBKIT "Option to use QT Webkit" ON)
    MARK_AS_ADVANCED(VTK_QT_USE_WEBKIT)
    IF( VTK_QT_USE_WEBKIT )
      QT4_WRAP_UI(UI_FILES vtkQtRichTextView.ui)
      SET(QVTKLibSrcs ${QVTKLibSrcs} ${UI_FILES} vtkQtRichTextView.cxx)
      SET(QVTKMocHeaders ${QVTKMocHeaders} vtkQtRichTextView.h)
      IF(QT_PHONON_FOUND AND APPLE)
        SET(QT_USE_PHONON 1)
      ENDIF(QT_PHONON_FOUND AND APPLE)
      SET(QT_USE_QTWEBKIT 1)
    ENDIF( VTK_QT_USE_WEBKIT )
  ENDIF(QT_QTWEBKIT_FOUND)
ENDIF(VTK_USE_VIEWS)
  
# help CMake find QtDesigner on Mac (binary install)
FIND_PATH(QT_QTDESIGNER_INCLUDE_DIR QDesignerComponents
          PATHS ${QT_LIBRARY_DIR}/QtDesigner.framework/Headers
          NO_DEFAULT_PATH)

# import Qt4 build settings
SET (QT_USE_QTNETWORK 1)
INCLUDE(${QT_USE_FILE})

SET ( PluginLibSrcs
      Q4VTKWidgetPlugin.cxx
      )

SET ( PluginMocHeaders
      Q4VTKWidgetPlugin.h
      )

# CMake apparently doesn't do this automatically
SET_SOURCE_FILES_PROPERTIES(Q4VTKWidgetPlugin.cxx PROPERTIES 
                            OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/QVTKWidget.xpm)

# Reduce the number of dirs that get included on moc command line
# since it causes issues on Windows 2000.
GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${MOC_INCLUDE_DIRS}")

QT4_WRAP_CPP ( QVTKLibMocSrcs ${QVTKMocHeaders} )

SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

ADD_LIBRARY(QVTK ${QVTKLibSrcs} ${QVTKLibMocSrcs})

# Apply user-defined properties to the library target.
IF(VTK_LIBRARY_PROPERTIES)
  SET_TARGET_PROPERTIES(QVTK PROPERTIES ${VTK_LIBRARY_PROPERTIES})
ENDIF(VTK_LIBRARY_PROPERTIES)

TARGET_LINK_LIBRARIES( QVTK 
  ${QT_LIBRARIES}
  vtkRendering 
  vtkGraphics
  vtkImaging 
  vtkCommon)

# add extra dependencies if necessary
TARGET_LINK_LIBRARIES( QVTK vtkQtChart )
IF(VTK_USE_VIEWS)
  TARGET_LINK_LIBRARIES( QVTK vtkViews )
ENDIF(VTK_USE_VIEWS)

IF(APPLE)
  IF(VTK_USE_CARBON)
    TARGET_LINK_LIBRARIES( QVTK "-framework Carbon" )
  ENDIF(VTK_USE_CARBON)
ENDIF(APPLE)

# build plugin
SET(QT_PLUGIN_LIBS ${QT_LIBRARIES})

ADD_DEFINITIONS(-DQT_PLUGIN)
# A release build of the designer must have 
# a release build of this plugin to work properly
# if the QtGui exists, a release build of the designer is assumed
IF(QT_QTGUI_LIBRARY)
  ADD_DEFINITIONS(-DQT_NO_DEBUG)
  SET(QT_PLUGIN_LIBS
     ${QT_QTCORE_LIBRARY}
     ${QT_QTGUI_LIBRARY}
  )
ENDIF(QT_QTGUI_LIBRARY)
INCLUDE_DIRECTORIES(${QT_QTDESIGNER_INCLUDE_DIR})
QT4_WRAP_CPP ( PluginMocSrcs ${PluginMocHeaders} )

# add QVTK plugin from sources 
# stand-alone as it doesn't depend on QVTK library
ADD_LIBRARY ( QVTKWidgetPlugin
  SHARED
  ${PluginLibSrcs}
  ${PluginMocSrcs}
)

# link with release version of Qt libs
TARGET_LINK_LIBRARIES( QVTKWidgetPlugin 
  ${QT_PLUGIN_LIBS}
)


# Configure the VTKConfigQt.cmake support file.
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/VTKConfigQt.cmake.in
               ${VTK_BINARY_DIR}/VTKConfigQt.cmake @ONLY IMMEDIATE)


# install rules

# The VTK_INSTALL_QT_PLUGIN_DIR variable sets the location
# in which the Qt plugin will be installed.  It may or may not contain
# variable references to CMAKE_INSTALL_PREFIX and VTK_INSTALL_QT_DIR.
# The default is to install to VTK_INSTALL_QT_DIR under the installation
# prefix.  The default VTK_INSTALL_QT_DIR will allow the designer plugin
# path to be set to vtk-install-prefix/plugins to get the plugin.
IF(NOT VTK_INSTALL_QT_DIR)
  SET(VTK_INSTALL_QT_DIR /plugins/designer)
ENDIF(NOT VTK_INSTALL_QT_DIR)

# If no runtime is to be installed then do not install the qt plugin.
IF(VTK_INSTALL_NO_RUNTIME)
  SET(VTK_INSTALL_NO_QT_PLUGIN 1)
ENDIF(VTK_INSTALL_NO_RUNTIME)

IF(NOT VTK_INSTALL_NO_QT_PLUGIN)
  # Set default plugin install directory.
  SET(DOLLAR "$")
  IF(DEFINED VTK_INSTALL_QT_PLUGIN_DIR)
  ELSE(DEFINED VTK_INSTALL_QT_PLUGIN_DIR)
    SET(VTK_INSTALL_QT_PLUGIN_DIR "${DOLLAR}{CMAKE_INSTALL_PREFIX}${DOLLAR}{VTK_INSTALL_QT_DIR}"
      CACHE STRING "Directory in which the VTK Qt plugin is placed during installation.")
    MARK_AS_ADVANCED(VTK_INSTALL_QT_PLUGIN_DIR)
  ENDIF(DEFINED VTK_INSTALL_QT_PLUGIN_DIR)

  # Configure the plugin install script.  This is used instead of
  # INSTALL TARGETS to allow the plugin to be installed outside the
  # main install prefix.  Attach the script as a post-install script.
  CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/PluginInstall.cmake.in
                 ${CMAKE_CURRENT_BINARY_DIR}/PluginInstall.cmake
                 @ONLY IMMEDIATE)
  SET_TARGET_PROPERTIES(QVTK PROPERTIES POST_INSTALL_SCRIPT
    ${CMAKE_CURRENT_BINARY_DIR}/PluginInstall.cmake
    )
ENDIF(NOT VTK_INSTALL_NO_QT_PLUGIN)

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES
    ${QVTKMocHeaders}
    ${QVTKNonMocHeaders}
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}
    COMPONENT Development
    )

  INSTALL(FILES
    "${VTK_BINARY_DIR}/VTKConfigQt.cmake"
    DESTINATION ${VTK_INSTALL_PACKAGE_DIR_CM24}
    COMPONENT Development
    )
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

IF(NOT VTK_INSTALL_NO_LIBRARIES)
  INSTALL(TARGETS QVTK
    RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT RuntimeLibraries
    LIBRARY DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT RuntimeLibraries
    ARCHIVE DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/CTestCustom.ctest.in"
  "${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.ctest" @ONLY IMMEDIATE)


# recurse into Chart directory
SUBDIRS(Chart)

# recurse into testing directory if testing is on
IF(BUILD_TESTING)
  SUBDIRS(Testing/Cxx)
ENDIF(BUILD_TESTING)

