PROJECT (Step2)

INCLUDE (${CMAKE_ROOT}/Modules/FindVTK.cmake)
IF (USE_VTK_FILE)
  INCLUDE(${USE_VTK_FILE})
ENDIF (USE_VTK_FILE)

LINK_LIBRARIES( 
  vtkRendering
  vtkGraphics
  vtkImaging
  vtkFiltering
  vtkCommon
)

ADD_EXECUTABLE(Cone2 Cone2.cxx)
