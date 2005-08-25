
# we need Qt3 or Qt4
SET(QT_MAX_VERSION "4.9.9")
INCLUDE( ${CMAKE_ROOT}/Modules/FindQt.cmake )

# set up sources to build
SET ( QVTKLibSrcs
    vtkEventQtSlotConnect.cxx
    QVTKWidget.cxx )

SET ( QVTKMocHeaders
    QVTKWidget.h 
    vtkEventQtSlotConnect.h)

INCLUDE_DIRECTORIES ( ${CMAKE_CURRENT_BINARY_DIR} )


IF(QT_INST_MAJOR_VERSION GREATER 3)

  # Qt4 settings
  INCLUDE_DIRECTORIES ( ${QT_INCLUDE_DIR} ${QT_QTCORE_INCLUDE_DIR} ${QT_QTGUI_INCLUDE_DIR})
  ADD_DEFINITIONS(-DQT_GUI_LIB -DQT_CORE_LIB -DQT_DLL)
  
  SET ( PluginLibSrcs
        Q4VTKWidgetPlugin.cxx
        QVTKWidget.cxx)

  SET ( PluginMocHeaders
        Q4VTKWidgetPlugin.h
        QVTKWidget.h)

  # CMake apparently doesn't do this automatically
  SET_SOURCE_FILES_PROPERTIES(Q4VTKWidgetPlugin.cxx PROPERTIES 
                              OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/QVTKWidget.xpm)

  QT4_WRAP_CPP ( QVTKLibMocSrcs ${QVTKMocHeaders} )
  QT4_WRAP_CPP ( PluginMocSrcs ${PluginMocHeaders} )

IF(APPLE)
# assume Qt framework
  SET(QT_LIBS "-framework QtGui" "-framework QtCore")
ELSE(APPLE)
  IF(CMAKE_BUILD_TYPE)
    SET(QT_LIBS optimized ${QT_QTGUI_LIBRARY} debug ${QT_QTGUI_LIBRARY_DEBUG}
                optimized ${QT_QTCORE_LIBRARY} debug ${QT_QTCORE_LIBRARY_DEBUG} )
  ELSE(CMAKE_BUILD_TYPE)
    IF(CMAKE_CONFIGURATION_TYPES)
      SET(QT_LIBS optimized ${QT_QTGUI_LIBRARY} debug ${QT_QTGUI_LIBRARY_DEBUG}
                  optimized ${QT_QTCORE_LIBRARY} debug ${QT_QTCORE_LIBRARY_DEBUG} )
    ELSE(CMAKE_CONFIGURATION_TYPES)
      SET(QT_LIBS ${QT_QTGUI_LIBRARY} ${QT_QTCORE_LIBRARY})
    ENDIF(CMAKE_CONFIGURATION_TYPES)
  ENDIF(CMAKE_BUILD_TYPE)
ENDIF(APPLE)
  
ELSE(QT_INST_MAJOR_VERSION GREATER 3)
  
  # Qt3 settings
  INCLUDE_DIRECTORIES ( ${QT_INCLUDE_DIR} )
  # assume Qt dll
  ADD_DEFINITIONS(-DQT_DLL)
  # check for a multithreaded Qt3 dll
  IF(QT_QT_LIBRARY MATCHES "mt")
    ADD_DEFINITIONS(-DQT_THREAD_SUPPORT)
  ENDIF(QT_QT_LIBRARY MATCHES "mt")
  
  SET ( PluginLibSrcs
        Q3VTKWidgetPlugin.cxx
        QVTKWidget.cxx)

  SET ( PluginMocHeaders
        QVTKWidget.h)

  # CMake apparently doesn't do this automatically
  SET_SOURCE_FILES_PROPERTIES(Q3VTKWidgetPlugin.cxx PROPERTIES 
                              OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/QVTKWidget.xpm)


  QT_WRAP_CPP ( QVTK QVTKLibMocSrcs ${QVTKMocHeaders} )
  QT_WRAP_CPP ( QVTKWidgetPlugin PluginMocSrcs ${PluginMocHeaders} )
  
  SET(QT_LIBS ${QT_QT_LIBRARY} )
  
ENDIF(QT_INST_MAJOR_VERSION GREATER 3)

SET_SOURCE_FILES_PROPERTIES(vtkEventQtSlotConnect.cxx PROPERTIES 
                            OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/moc_vtkEventQtSlotConnect.cxx)

ADD_LIBRARY(QVTK ${QVTKLibSrcs} ${QVTKLibMocSrcs})

# Apply user-defined properties to the library target.
IF(VTK_LIBRARY_PROPERTIES)
  SET_TARGET_PROPERTIES(QVTK PROPERTIES ${VTK_LIBRARY_PROPERTIES})
ENDIF(VTK_LIBRARY_PROPERTIES)

# give Qt framework directory for linking
IF(APPLE)
IF(QT_INST_MAJOR_VERSION GREATER 3)
  SET_TARGET_PROPERTIES(QVTK PROPERTIES LINK_FLAGS "-F${QT_LIBRARY_DIR}")
ENDIF(QT_INST_MAJOR_VERSION GREATER 3)
ENDIF(APPLE)

TARGET_LINK_LIBRARIES( QVTK 
  ${QT_LIBS}
  vtkRendering 
  vtkGraphics
  vtkImaging 
  vtkCommon)


# build plugin by default
# even if VTK is built as static libraries (for convenience)
SET(BUILD_QVTK_PLUGIN ON)

# don't build plugin on systems that require dependents to be shared (PIC actually)
IF(NOT BUILD_SHARED_LIBS)
  IF(CMAKE_SYSTEM MATCHES "HP-UX.*")
    SET(BUILD_QVTK_PLUGIN OFF)
  ENDIF(CMAKE_SYSTEM MATCHES "HP-UX.*")
ENDIF(NOT BUILD_SHARED_LIBS)


IF(BUILD_QVTK_PLUGIN)

  ADD_DEFINITIONS(-DQT_PLUGIN)
  IF(QT_INST_MAJOR_VERSION GREATER 3)
    # A release build of the designer must have 
    # a release build of this plugin to work properly
    # if the QtGui exists, a release build of the designer is assumed
    IF(QT_QTGUI_LIBRARY)
      ADD_DEFINITIONS(-DQT_NO_DEBUG)
    ENDIF(QT_QTGUI_LIBRARY)
  ENDIF(QT_INST_MAJOR_VERSION GREATER 3)
  
# add QVTK plugin from sources 
# stand-alone as it doesn't depend on QVTK library
  ADD_LIBRARY ( QVTKWidgetPlugin
    SHARED
    ${PluginLibSrcs}
    ${PluginMocSrcs}
  )

IF(APPLE)
IF(QT_INST_MAJOR_VERSION GREATER 3)
  SET_TARGET_PROPERTIES(QVTKWidgetPlugin PROPERTIES LINK_FLAGS "-F${QT_LIBRARY_DIR}")
ENDIF(QT_INST_MAJOR_VERSION GREATER 3)
ENDIF(APPLE)

# link with release version of Qt libs
  TARGET_LINK_LIBRARIES( QVTKWidgetPlugin 
    ${QT_LIBS}
    vtkRendering
    vtkImaging
    vtkGraphics
    vtkFiltering
    vtkCommon
  )
ELSE(BUILD_QVTK_PLUGIN)
  MESSAGE(STATUS "Will not build QVTK plugin because VTK isn't build as shared libraries")
ENDIF(BUILD_QVTK_PLUGIN)


# Configure the VTKConfigQt.cmake support file.
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/VTKConfigQt.cmake.in
               ${VTK_BINARY_DIR}/VTKConfigQt.cmake @ONLY IMMEDIATE)


# install rules

IF(BUILD_QVTK_PLUGIN)
# install at /plugins/designer allows one to set their designer plugin path
# in the designer preferences to the vtk-install/plugins directory.
  IF(NOT VTK_INSTALL_QT_DIR)
    SET(VTK_INSTALL_QT_DIR /plugins/designer)
  ENDIF(NOT VTK_INSTALL_QT_DIR)
  IF(NOT VTK_INSTALL_NO_RUNTIME)
    INSTALL_TARGETS( ${VTK_INSTALL_QT_DIR} QVTKWidgetPlugin )
  ENDIF(NOT VTK_INSTALL_NO_RUNTIME)

  IF ( UNIX )
    ADD_CUSTOM_TARGET( install-qtplugin
                       install -m 777 -o root ${VTK_BINARY_DIR}/bin/libQVTKWidgetPlugin.so ${QTDIR}/${VTK_INSTALL_QT_DIR}
                       DEPENDS ${VTK_BINARY_DIR}/bin/libQVTKWidgetPlugin.so )
  ENDIF ( UNIX )
ENDIF(BUILD_QVTK_PLUGIN)

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL_FILES( ${VTK_INSTALL_INCLUDE_DIR} .h ${QVTKLibSrcs})
  INSTALL_FILES( ${VTK_INSTALL_PACKAGE_DIR} FILES ${VTK_BINARY_DIR}/VTKConfigQt.cmake )
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
IF(NOT VTK_INSTALL_NO_LIBRARIES)
  INSTALL_TARGETS( ${VTK_INSTALL_LIB_DIR} QVTK )
ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)

