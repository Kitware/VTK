IF (UNIX)

PROJECT (MotifExamples)

# WARNING: The Motif library HAS to be placed BEFORE the Xt library brought in
# by VTK (see Motif FAQ item 335)

LINK_LIBRARIES(-lXm)

INCLUDE (${CMAKE_ROOT}/Modules/FindVTK.cmake)
IF (USE_VTK_FILE)
  INCLUDE(${USE_VTK_FILE})
ENDIF (USE_VTK_FILE)

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
