PROJECT (Step1)

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

ADD_EXECUTABLE(Cone Cone.cxx)
