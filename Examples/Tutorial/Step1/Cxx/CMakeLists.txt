PROJECT (Step1)

INCLUDE (${CMAKE_ROOT}/Modules/FindVTK.cmake)
INCLUDE (${CMAKE_ROOT}/Modules/UseVTKIncludes.cmake)
INCLUDE (${CMAKE_ROOT}/Modules/UseVTKLibraries.cmake)

LINK_LIBRARIES( 
  vtkRendering
  vtkGraphics
  vtkImaging
  vtkIO
  vtkFiltering
  vtkCommon
)

ADD_EXECUTABLE(Cone Cone.cxx)
