PROJECT (Rendering)

INCLUDE (${CMAKE_ROOT}/Modules/FindVTK.cmake)
IF (USE_VTK_FILE)
  INCLUDE(${USE_VTK_FILE})
ENDIF (USE_VTK_FILE)

ADD_EXECUTABLE(Cylinder Cylinder.cxx)
TARGET_LINK_LIBRARIES(Cylinder vtkRendering)

ADD_EXECUTABLE(SpecularSpheres SpecularSpheres.cxx)
TARGET_LINK_LIBRARIES(SpecularSpheres vtkRendering)

ADD_EXECUTABLE(DiffuseSpheres DiffuseSpheres.cxx)
TARGET_LINK_LIBRARIES(DiffuseSpheres vtkRendering)

ADD_EXECUTABLE(AmbientSpheres AmbientSpheres.cxx)
TARGET_LINK_LIBRARIES(AmbientSpheres vtkRendering)

