SET(KIT GUISupport)
SET(UKIT RENDERING)


# Determine Qt GUI.
IF(APPLE)
# must use Carbon on the Mac to interface with Qt
VTK_DEPENDENT_OPTION(VTK_USE_QVTK "Build QVTK widget and plugin for Qt" OFF
                     "VTK_USE_RENDERING;VTK_USE_CARBON" OFF)
ELSE(APPLE)
VTK_DEPENDENT_OPTION(VTK_USE_QVTK "Build QVTK widget and plugin for Qt" OFF
                     "VTK_USE_RENDERING" OFF)
ENDIF(APPLE)

SET(KIT_LIBS vtkRendering vtkGraphics vtkImaging vtkCommon)

IF(VTK_USE_QVTK)
  # we need Qt
  INCLUDE( ${CMAKE_ROOT}/Modules/FindQt.cmake )

  # set up sources to build
  SET ( QVTKLibSrcs
      QVTKWidget.cxx)
  SET ( QVTKMocHeaders
      QVTKWidget.h)
  SET ( PluginLibSrcs
      QVTKWidget.cxx
      QVTKWidgetPlugin.cxx)
  SET ( PluginMocHeaders
      QVTKWidget.h)
  
  # CMake apparently doesn't do this automatically
  SET_SOURCE_FILES_PROPERTIES(QVTKWidgetPlugin.cxx PROPERTIES 
                             OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/QVTKWidget.xpm)
 
  # assuming a Qt dll
  ADD_DEFINITIONS(-DQT_DLL -DQT_PLUGIN)

  # a multithreaded Qt dll
  IF(${QT_QT_LIBRARY} MATCHES "mt")
    ADD_DEFINITIONS(-DQT_THREAD_SUPPORT)
  ENDIF(${QT_QT_LIBRARY} MATCHES "mt")

  # include Qt
  INCLUDE_DIRECTORIES ( ${QT_INCLUDE_DIR} )

  QT_WRAP_CPP ( QVTK QVTKLibMocSrcs ${QVTKMocHeaders} )
  QT_WRAP_CPP ( QVTKWidgetPlugin PluginMocSrcs ${PluginMocHeaders} )

  SET(Kit_SRCS ${Kit_SRCS} ${QVTKLibSrcs} ${QVTKLibMocSrcs})

  SET(KIT_LIBS ${QT_QT_LIBRARY} ${KIT_LIBS})
  INSTALL_FILES(/include/vtk QVTKWidget.h)

  # add QVTK plugin from sources 
  # stand-alone as it doesn't depend on QVTK library
  ADD_LIBRARY ( QVTKWidgetPlugin
    SHARED
    ${PluginLibSrcs}
    ${PluginMocSrcs}
  )
  TARGET_LINK_LIBRARIES( QVTKWidgetPlugin 
    ${QT_QT_LIBRARY}
    vtkRendering
    vtkImaging
    vtkGraphics
    vtkFiltering
    vtkCommon
  )

  # install rules
  INSTALL_TARGETS( /plugins/designer QVTKWidgetPlugin )
  
  IF ( UNIX )
    ADD_CUSTOM_TARGET( install-qtplugin
                       install -m 777 -o root ${VTK_BINARY_DIR}/bin/libQVTKWidgetPlugin.so ${QTDIR}/plugins/designer
                       DEPENDS ${VTK_BINARY_DIR}/bin/libQVTKWidgetPlugin.so )
  ENDIF ( UNIX )

ENDIF(VTK_USE_QVTK)


ADD_LIBRARY(vtk${KIT} ${Kit_SRCS})

TARGET_LINK_LIBRARIES( vtk${KIT} ${KIT_LIBS} )

INSTALL_TARGETS( /lib/vtk vtk${KIT} )

