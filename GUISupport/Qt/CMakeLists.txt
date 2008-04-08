
INCLUDE_REGULAR_EXPRESSION("^(vtk|Q).*$")

# we need Qt3 or Qt4
# we ask up front so FindQt.cmake doesn't auto-find an undesired Qt version
SET(DESIRED_QT_VERSION 0 CACHE STRING "Pick a version of Qt to use: 3 or 4")

IF(DESIRED_QT_VERSION MATCHES 0)
  MESSAGE("Please set the DESIRED_QT_VERSION")
ELSE(DESIRED_QT_VERSION MATCHES 0)
  
SET ( QT_USE_QTSQL 1 )

INCLUDE( ${CMAKE_ROOT}/Modules/FindQt${DESIRED_QT_VERSION}.cmake )

IF(NOT QT_FOUND)
  MESSAGE("Qt was not found")
ELSE(NOT QT_FOUND)

# set up sources to build
SET ( QVTKLibSrcs
    vtkEventQtSlotConnect.cxx
    vtkQtConnection.cxx
    QVTKWidget.cxx
    )

SET ( QVTKMocHeaders
    ${CMAKE_CURRENT_BINARY_DIR}/QVTKWidget.h 
    vtkQtConnection.h
    )

INCLUDE_DIRECTORIES ( ${CMAKE_CURRENT_BINARY_DIR} )


IF(DESIRED_QT_VERSION MATCHES 4)

  SET(Q_SIGNALS Q_SIGNALS)
  SET(Q_SLOTS Q_SLOTS)

  # add files that require Qt 4
  SET ( QVTKLibSrcs
      ${QVTKLibSrcs}
      vtkQtAbstractModelAdapter.cxx
      vtkQtInitialization.cxx
      vtkQtSQLDatabase.cxx
      vtkQtSQLQuery.cxx
      vtkQtTableModelAdapter.cxx
      vtkQtTimePointUtility.cxx
      vtkQtTreeModelAdapter.cxx 
      )
    
  SET ( QVTKMocHeaders
      ${QVTKMocHeaders}
      vtkQtAbstractModelAdapter.h
      vtkQtTableModelAdapter.h
      vtkQtTreeModelAdapter.h
      )

  # add additional files depending on infovis and/or views
  IF(VTK_USE_VIEWS)
    SET( QVTKLibSrcs ${QVTKLibSrcs}
      vtkQtItemView.cxx
      vtkQtListView.cxx
      vtkQtTableView.cxx
      vtkQtTreeView.cxx
      ) 
    SET ( QVTKMocHeaders ${QVTKMocHeaders}
      vtkQtItemView.h
      )
  ENDIF(VTK_USE_VIEWS)
  
  # help CMake fine QtDesigner on Mac (binary install)
  FIND_PATH(QT_QTDESIGNER_INCLUDE_DIR QDesignerComponents
            PATHS ${QT_LIBRARY_DIR}/QtDesigner.framework/Headers
            NO_DEFAULT_PATH)

  # import Qt4 build settings
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

  QT4_WRAP_CPP ( QVTKLibMocSrcs ${QVTKMocHeaders} )

  SET(QT_LIBS "${QT_LIBRARIES}")
  
ELSE(DESIRED_QT_VERSION MATCHES 4)
  
  SET(Q_SIGNALS signals)
  SET(Q_SLOTS slots)

  # Qt3 settings
  INCLUDE_DIRECTORIES ( ${QT_INCLUDE_DIR} )
  # assume Qt dll
  ADD_DEFINITIONS(-DQT_DLL)
  # check for a multithreaded Qt3 dll
  IF(QT_QT_LIBRARY MATCHES "mt")
    ADD_DEFINITIONS(-DQT_THREAD_SUPPORT)
  ENDIF(QT_QT_LIBRARY MATCHES "mt")
  
  SET ( PluginLibSrcs
        Q3VTKWidgetPlugin.cxx)

  SET ( PluginMocHeaders
        Q3VTKWidgetPlugin.h
        )

  # CMake apparently doesn't do this automatically
  SET_SOURCE_FILES_PROPERTIES(Q3VTKWidgetPlugin.cxx PROPERTIES 
                              OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/QVTKWidget.xpm)


  QT_WRAP_CPP ( QVTK QVTKLibMocSrcs ${QVTKMocHeaders} )
  
  SET(QT_LIBS ${QT_QT_LIBRARY})

ENDIF(DESIRED_QT_VERSION MATCHES 4)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/QVTKWidget.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/QVTKWidget.h
               @ONLY)

ADD_LIBRARY(QVTK ${QVTKLibSrcs} ${QVTKLibMocSrcs})

# Apply user-defined properties to the library target.
IF(VTK_LIBRARY_PROPERTIES)
  SET_TARGET_PROPERTIES(QVTK PROPERTIES ${VTK_LIBRARY_PROPERTIES})
ENDIF(VTK_LIBRARY_PROPERTIES)

TARGET_LINK_LIBRARIES( QVTK 
  ${QT_LIBS}
  vtkRendering 
  vtkGraphics
  vtkImaging 
  vtkCommon)

# add extra dependencies if necessary
IF(DESIRED_QT_VERSION MATCHES 4)
  IF(VTK_USE_VIEWS)
    TARGET_LINK_LIBRARIES( QVTK vtkViews )
  ENDIF(VTK_USE_VIEWS)
ENDIF(DESIRED_QT_VERSION MATCHES 4)

# build plugin
SET(QT_PLUGIN_LIBS ${QT_LIBS})

ADD_DEFINITIONS(-DQT_PLUGIN)
IF(DESIRED_QT_VERSION MATCHES 4)
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
ELSE(DESIRED_QT_VERSION MATCHES 4)
  QT_WRAP_CPP ( QVTKWidgetPlugin PluginMocSrcs ${PluginMocHeaders} )
ENDIF(DESIRED_QT_VERSION MATCHES 4)

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
    "${CMAKE_CURRENT_BINARY_DIR}/QVTKWidget.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/QVTKWin32Header.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/vtkEventQtSlotConnect.h"
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

# recurse into testing directory if testing is on
IF(DESIRED_QT_VERSION MATCHES 4)
  IF(BUILD_TESTING)
    SUBDIRS(Testing/Cxx)
  ENDIF(BUILD_TESTING)
ENDIF(DESIRED_QT_VERSION MATCHES 4)

ENDIF(NOT QT_FOUND)

ENDIF(DESIRED_QT_VERSION MATCHES 0)

