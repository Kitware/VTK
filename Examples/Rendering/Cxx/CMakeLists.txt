PROJECT (Rendering)

FIND_PACKAGE(VTK)
IF(NOT VTK_DIR)
  MESSAGE(FATAL_ERROR "Please set VTK_DIR.")
ENDIF(NOT VTK_DIR)
INCLUDE(${VTK_USE_FILE})

ADD_EXECUTABLE(Cylinder Cylinder.cxx)
TARGET_LINK_LIBRARIES(Cylinder vtkRendering)

ADD_EXECUTABLE(SpecularSpheres SpecularSpheres.cxx)
TARGET_LINK_LIBRARIES(SpecularSpheres vtkRendering)

ADD_EXECUTABLE(DiffuseSpheres DiffuseSpheres.cxx)
TARGET_LINK_LIBRARIES(DiffuseSpheres vtkRendering)

ADD_EXECUTABLE(AmbientSpheres AmbientSpheres.cxx)
TARGET_LINK_LIBRARIES(AmbientSpheres vtkRendering)

