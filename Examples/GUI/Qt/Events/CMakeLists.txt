PROJECT(QtEvents)

FIND_PACKAGE(VTK)
IF(NOT VTK_DIR)
  MESSAGE(FATAL_ERROR "Please set VTK_DIR.")
ENDIF(NOT VTK_DIR)
INCLUDE(${VTK_USE_FILE})

INCLUDE (${CMAKE_ROOT}/Modules/FindQt.cmake)

SET (SRCS
  main.cxx
)
SET (MOC_HDRS
)

INCLUDE_DIRECTORIES(
        ${QT_INCLUDE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

QT_WRAP_UI( qtevents UI_HDRS UI_SRCS GUI.ui )
#QT_WRAP_CPP( qtevents MOC_SRCS ${MOC_HDRS} )

# help cmake with Qt depends
SET_SOURCE_FILES_PROPERTIES(GUI.cxx PROPERTIES
                            OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/GUI.ui.h)

# It's nice to have the ui in the windows project file...just double click on it
# and designer comes up on that ui file :)
#IF (${CMAKE_BUILD_TOOL} MATCHES "msdev")
  #SET (SimpleViewSrcs ${SimpleViewSrcs} uiSimpleView.ui)
#ENDIF (${CMAKE_BUILD_TOOL} MATCHES "msdev")
#IF (${CMAKE_BUILD_TOOL} MATCHES "devenv")
  #SET (SimpleViewSrcs ${SimpleViewSrcs} uiSimpleView.ui)
#ENDIF (${CMAKE_BUILD_TOOL} MATCHES "devenv")

ADD_EXECUTABLE( qtevents ${SRCS} ${UI_SRCS})

# note: if you use Mesa in VTK and get link errors about opengl symbols
#       they are coming from Qt.  Either link OpenGL in, configure Qt to
#       dlopen the GL library, or configure Qt without OpenGL support.


TARGET_LINK_LIBRARIES( qtevents
  QVTK
  ${QT_QT_LIBRARY}
  vtkRendering
  vtkGraphics
  vtkIO
  vtkCommon
)


