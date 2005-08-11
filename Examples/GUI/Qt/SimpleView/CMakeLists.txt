PROJECT(QtSimpleView)

FIND_PACKAGE(VTK)
IF(NOT VTK_DIR)
  MESSAGE(FATAL_ERROR "Please set VTK_DIR.")
ENDIF(NOT VTK_DIR)
INCLUDE(${VTK_USE_FILE})

# use what QVTK built with
SET(QT_MOC_EXECUTABLE ${VTK_QT_MOC_EXECUTABLE} CACHE FILEPATH "")
SET(QT_UIC_EXECUTABLE ${VTK_QT_UIC_EXECUTABLE} CACHE FILEPATH "")
SET(QT_QMAKE_EXECUTABLE ${VTK_QT_QMAKE_EXECUTABLE} CACHE FILEPATH "")
SET(QT_MAX_VERSION "4.9.9")
FIND_PACKAGE(Qt)

SET (SimpleViewSrcs
  main.cxx
)

# Use the include path and library for Qt that is used by VTK.
INCLUDE_DIRECTORIES(
  ${QT_INCLUDE_DIR}
  ${QT_QTGUI_INCLUDE_DIR}
  ${QT_QTCORE_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

IF(QT_INST_MAJOR_VERSION GREATER 3)
  
  SET(SimpleViewSrcs ${SimpleViewSrcs} SimpleView4.cxx)
  SET(UIS uiSimpleView4.ui)
  SET (SimpleViewHeaders SimpleView4.h)
  QT4_WRAP_UI(UIHeaders ${UIS})
  QT4_WRAP_CPP(MOCSrcs ${SimpleViewHeaders} )
  ADD_DEFINITIONS(-DQT_GUI_LIBS -DQT_CORE_LIB -DQT3_SUPPORT)
  SET_SOURCE_FILES_PROPERTIES(${SimpleViewSrcs} PROPERTIES
                            OBJECT_DEPENDS "${UIHeaders}")

ELSE(QT_INST_MAJOR_VERSION GREATER 3)
  
  SET(SimpleViewSrcs ${SimpleViewSrcs} SimpleView3.cxx)
  SET(UIS uiSimpleView3.ui)
  SET (SimpleViewHeaders SimpleView3.h)
  QT_WRAP_UI( simpleview UIHeaders UISrcs ${UIS})
  QT_WRAP_CPP( simpleview MOCSrcs ${SimpleViewHeaders} )
  SET_SOURCE_FILES_PROPERTIES(${SimpleViewSrcs} PROPERTIES
                            OBJECT_DEPENDS "${UIHeaders}")

ENDIF(QT_INST_MAJOR_VERSION GREATER 3)

# It's nice to have the ui in the windows project file...just double click on it
# and designer comes up on that ui file :)
IF (${CMAKE_BUILD_TOOL} MATCHES "msdev")
  SET (SimpleViewSrcs ${SimpleViewSrcs} ${UIS})
ENDIF (${CMAKE_BUILD_TOOL} MATCHES "msdev")
IF (${CMAKE_BUILD_TOOL} MATCHES "devenv")
  SET (SimpleViewSrcs ${SimpleViewSrcs} ${UIS})
ENDIF (${CMAKE_BUILD_TOOL} MATCHES "devenv")

ADD_EXECUTABLE( qtsimpleview MACOSX_BUNDLE ${SimpleViewSrcs} ${UISrcs} ${MOCSrcs})

# note: if you use Mesa in VTK and get link errors about opengl symbols
#       they are coming from Qt.  Either link OpenGL in, configure Qt to 
#       dlopen the GL library, or configure Qt without OpenGL support.

IF(QT_INST_MAJOR_VERSION GREATER 3)
  IF(APPLE)
    SET(QT_LIBS   "-framework QtGui" "-framework QtCore")
    SET_TARGET_PROPERTIES(qtimageviewer PROPERTIES LINK_FLAGS "-F${QT_LIBRARY_DIR}")
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
  SET(QT_LIBS   ${QT_QT_LIBRARY})
ENDIF(QT_INST_MAJOR_VERSION GREATER 3)

TARGET_LINK_LIBRARIES( qtsimpleview
  QVTK
  ${QT_LIBS}
  vtkRendering
  vtkGraphics
  vtkIO
  vtkCommon
)


