PROJECT(QtEvents)

FIND_PACKAGE(VTK)
IF(NOT VTK_DIR)
  MESSAGE(FATAL_ERROR "Please set VTK_DIR.")
ENDIF(NOT VTK_DIR)
INCLUDE(${VTK_USE_FILE})

# use what QVTK built with
SET(QT_MOC_EXECUTABLE ${VTK_QT_MOC_EXECUTABLE})
SET(QT_UIC_EXECUTABLE ${VTK_QT_UIC_EXECUTABLE})

SET (SRCS
  main.cxx
)

# Use the include path and library for Qt that is used by VTK.
INCLUDE_DIRECTORIES(
        ${VTK_QT_INCLUDE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

QT_WRAP_UI( qtevents UI_HDRS UI_SRCS GUI.ui )

# help cmake with Qt depends
SET_SOURCE_FILES_PROPERTIES(GUI.cxx PROPERTIES
                            OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/GUI.ui.h)

ADD_EXECUTABLE( qtevents ${SRCS} ${UI_SRCS})

# note: if you use Mesa in VTK and get link errors about opengl symbols,
#       those errors are coming from the Qt library needing to link with OpenGL.
#       Either link OpenGL in, configure Qt to dlopen the GL library, 
#       or configure Qt without OpenGL support.


TARGET_LINK_LIBRARIES( qtevents
  QVTK
  ${VTK_QT_QT_LIBRARY}
  vtkRendering
  vtkGraphics
  vtkIO
  vtkCommon
)


