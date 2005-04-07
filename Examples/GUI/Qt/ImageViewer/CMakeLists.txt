PROJECT(QtImageViewer)

FIND_PACKAGE(VTK)
IF(NOT VTK_DIR)
  MESSAGE(FATAL_ERROR "Please set VTK_DIR.")
ENDIF(NOT VTK_DIR)
INCLUDE(${VTK_USE_FILE})

SET (SRCS
  main.cxx
)

# Use the include path and library for Qt that is used by VTK.
INCLUDE_DIRECTORIES( ${VTK_QT_INCLUDE_DIR} )

ADD_EXECUTABLE( qtimageviewer ${SRCS})

# note: if you use Mesa in VTK and get link errors about opengl symbols
#       they are coming from Qt.  Either link OpenGL in, configure Qt to
#       dlopen the GL library, or configure Qt without OpenGL support.

TARGET_LINK_LIBRARIES( qtimageviewer
  QVTK
  ${VTK_QT_QT_LIBRARY}
  vtkRendering
  vtkGraphics
  vtkIO
  vtkCommon
)


