
# we need Qt
INCLUDE( ${CMAKE_ROOT}/Modules/FindQt.cmake )

# set up sources to build
SET ( QVTKLibSrcs
    QVTKWidget.cxx
    vtkEventQtSlotConnect.cxx)

SET ( QVTKMocHeaders
    QVTKWidget.h)

SET ( QVTKExtraMoc
    vtkEventQtSlotConnect.cxx)

SET ( PluginLibSrcs
    QVTKWidget.cxx
    QVTKWidgetPlugin.cxx)

SET ( PluginMocHeaders
    QVTKWidget.h)

# CMake apparently doesn't do this automatically
SET_SOURCE_FILES_PROPERTIES(QVTKWidgetPlugin.cxx PROPERTIES 
                            OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/QVTKWidget.xpm)

# help CMake with this generated moc file
SET_SOURCE_FILES_PROPERTIES(vtkEventQtSlotConnect.cxx PROPERTIES 
                            OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/moc_vtkEventQtSlotConnect.cxx)

# assuming a Qt dll, otherwise plugins aren't possible
ADD_DEFINITIONS(-DQT_DLL -DQT_PLUGIN)

# check for a multithreaded Qt dll
IF(QT_QT_LIBRARY MATCHES "mt")
  ADD_DEFINITIONS(-DQT_THREAD_SUPPORT)
ENDIF(QT_QT_LIBRARY MATCHES "mt")

# include Qt
INCLUDE_DIRECTORIES ( ${QT_INCLUDE_DIR} )
INCLUDE_DIRECTORIES ( ${CMAKE_CURRENT_BINARY_DIR} )

QT_WRAP_CPP ( QVTK QVTKLibMocSrcs ${QVTKMocHeaders} )
QT_WRAP_CPP ( QVTK QVTKLibExtraMocSrcs ${QVTKExtraMoc} )
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

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL_FILES( ${VTK_INSTALL_INCLUDE_DIR} .h ${QVTKLibSrcs})
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
IF(NOT VTK_INSTALL_NO_LIBRARIES)
  INSTALL_TARGETS( ${VTK_INSTALL_LIB_DIR} QVTK )
ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)

