
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
IF(QT_QT_LIBRARY MATCHES "mt")
  ADD_DEFINITIONS(-DQT_THREAD_SUPPORT)
ENDIF(QT_QT_LIBRARY MATCHES "mt")

# include Qt
INCLUDE_DIRECTORIES ( ${QT_INCLUDE_DIR} )

QT_WRAP_CPP ( QVTK QVTKLibMocSrcs ${QVTKMocHeaders} )
QT_WRAP_CPP ( QVTKWidgetPlugin PluginMocSrcs ${PluginMocHeaders} )

ADD_LIBRARY(QVTK ${QVTKLibSrcs} ${QVTKLibMocSrcs})

TARGET_LINK_LIBRARIES( QVTK ${QT_QT_LIBRARY} 
  vtkRendering 
  vtkGraphics
  vtkImaging 
  vtkCommon)

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



INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} QVTKWidget.h)
INSTALL_TARGETS( ${VTK_INSTALL_LIB_DIR} QVTK )

