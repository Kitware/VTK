PROJECT(QtImageViewer)

FIND_PACKAGE(VTK)
IF(NOT VTK_DIR)
  MESSAGE(FATAL_ERROR "Please set VTK_DIR.")
ENDIF(NOT VTK_DIR)
INCLUDE(${VTK_USE_FILE})

INCLUDE (${CMAKE_ROOT}/Modules/FindQt.cmake)

SET (SRCS
  main.cxx
)

INCLUDE_DIRECTORIES( ${QT_INCLUDE_DIR} )

ADD_EXECUTABLE( qtimageviewer ${SRCS})

# note: if you use Mesa in VTK and get link errors about opengl symbols
#       they are coming from Qt.  Either link OpenGL in, configure Qt to
#       dlopen the GL library, or configure Qt without OpenGL support.

TARGET_LINK_LIBRARIES( qtimageviewer
  QVTK
  ${QT_QT_LIBRARY}
  vtkRendering
  vtkGraphics
  vtkIO
  vtkCommon
)


