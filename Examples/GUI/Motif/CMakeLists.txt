IF (UNIX)

PROJECT (MotifExamples)

# WARNING: The Motif library HAS to be placed BEFORE the Xt library brought in
# by VTK (see Motif FAQ item 335)

LINK_LIBRARIES(-lXm)

FIND_PACKAGE(VTK)
IF(NOT VTK_DIR)
  MESSAGE(FATAL_ERROR "Please set VTK_DIR.")
ENDIF(NOT VTK_DIR)
INCLUDE(${VTK_USE_FILE})

LINK_LIBRARIES( 
  vtkRendering
  vtkGraphics
  vtkImaging
  vtkIO
  vtkFiltering
  vtkCommon
)

ADD_EXECUTABLE(Example1 Example1.cxx)
ADD_EXECUTABLE(Example2 Example2.cxx)

ENDIF (UNIX)
