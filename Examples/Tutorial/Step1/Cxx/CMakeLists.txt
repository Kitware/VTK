PROJECT (Step1)

INCLUDE (${CMAKE_ROOT}/Modules/FindVTK.cmake)
IF (USE_INSTALLED_VTK)
  INCLUDE (${VTK_INSTALL_DIR}/UseVTK.cmake)
ELSE (USE_INSTALLED_VTK)
  INCLUDE (${VTK_SOURCE_DIR}/UseVTK.cmake)
ENDIF (USE_INSTALLED_VTK)

LINK_LIBRARIES( 
  vtkRendering
  vtkGraphics
  vtkImaging
  vtkIO
  vtkFiltering
  vtkCommon
)

ADD_EXECUTABLE(Cone Cone.cxx)
