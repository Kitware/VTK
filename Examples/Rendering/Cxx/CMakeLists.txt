PROJECT (Rendering)

FIND_PACKAGE(VTK REQUIRED)
IF(NOT VTK_USE_RENDERING)
  MESSAGE(FATAL_ERROR "Example ${PROJECT_NAME} requires VTK_USE_RENDERING.")
ENDIF(NOT VTK_USE_RENDERING)
INCLUDE(${VTK_USE_FILE})

ADD_EXECUTABLE(Cylinder Cylinder.cxx)
TARGET_LINK_LIBRARIES(Cylinder vtkRendering)

ADD_EXECUTABLE(SpecularSpheres SpecularSpheres.cxx)
TARGET_LINK_LIBRARIES(SpecularSpheres vtkRendering)

ADD_EXECUTABLE(DiffuseSpheres DiffuseSpheres.cxx)
TARGET_LINK_LIBRARIES(DiffuseSpheres vtkRendering)

ADD_EXECUTABLE(AmbientSpheres AmbientSpheres.cxx)
TARGET_LINK_LIBRARIES(AmbientSpheres vtkRendering)

