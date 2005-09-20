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
SET(DESIRED_QT_VERSION ${VTK_DESIRED_QT_VERSION} CACHE FILEPATH "")
FIND_PACKAGE(Qt)
IF(QT_USE_FILE)
  INCLUDE(${QT_USE_FILE})
ELSE(QT_USE_FILE)
  SET(QT_LIBRARIES   ${QT_QT_LIBRARY})
ENDIF(QT_USE_FILE)

SET (SimpleViewSrcs
  main.cxx
)

# Use the include path and library for Qt that is used by VTK.
INCLUDE_DIRECTORIES(
  ${QT_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

IF(DESIRED_QT_VERSION MATCHES 4)
  
  SET(SimpleViewSrcs ${SimpleViewSrcs} SimpleView4.cxx)
  SET(UIS uiSimpleView4.ui)
  SET (SimpleViewHeaders SimpleView4.h)
  QT4_WRAP_UI(UIHeaders ${UIS})
  QT4_WRAP_CPP(MOCSrcs ${SimpleViewHeaders} )
  ADD_DEFINITIONS(-DQT_GUI_LIBS -DQT_CORE_LIB -DQT3_SUPPORT)
  SET_SOURCE_FILES_PROPERTIES(${SimpleViewSrcs} PROPERTIES
                            OBJECT_DEPENDS "${UIHeaders}")

ELSE(DESIRED_QT_VERSION MATCHES 4)
  
  SET(SimpleViewSrcs ${SimpleViewSrcs} SimpleView3.cxx)
  SET(UIS uiSimpleView3.ui)
  SET (SimpleViewHeaders SimpleView3.h)
  QT_WRAP_UI( simpleview UIHeaders UISrcs ${UIS})
  QT_WRAP_CPP( simpleview MOCSrcs ${SimpleViewHeaders} )
  SET_SOURCE_FILES_PROPERTIES(${SimpleViewSrcs} PROPERTIES
                            OBJECT_DEPENDS "${UIHeaders}")

ENDIF(DESIRED_QT_VERSION MATCHES 4)

# It's nice to have the ui in the windows project file...just double click on it
# and designer comes up on that ui file :)
IF (${CMAKE_BUILD_TOOL} MATCHES "msdev")
  SET (SimpleViewSrcs ${SimpleViewSrcs} ${UIS})
ENDIF (${CMAKE_BUILD_TOOL} MATCHES "msdev")
IF (${CMAKE_BUILD_TOOL} MATCHES "devenv")
  SET (SimpleViewSrcs ${SimpleViewSrcs} ${UIS})
ENDIF (${CMAKE_BUILD_TOOL} MATCHES "devenv")

ADD_EXECUTABLE( qtsimpleview MACOSX_BUNDLE ${SimpleViewSrcs} ${UISrcs} ${MOCSrcs})

TARGET_LINK_LIBRARIES( qtsimpleview
  QVTK
  ${QT_LIBRARIES}
  vtkRendering
  vtkGraphics
  vtkIO
  vtkCommon
)


