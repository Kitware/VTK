PROJECT (AMR)

FIND_PACKAGE(VTK REQUIRED)
IF(NOT VTK_USE_RENDERING)
  MESSAGE(FATAL_ERROR "Example ${PROJECT_NAME} requires VTK_USE_RENDERING.")
ENDIF(NOT VTK_USE_RENDERING)
INCLUDE(${VTK_USE_FILE})

ADD_EXECUTABLE(HierarchicalBox HierarchicalBox.cxx)
TARGET_LINK_LIBRARIES(HierarchicalBox vtkRendering vtkIO)

ADD_EXECUTABLE(HierarchicalBoxPipeline HierarchicalBoxPipeline.cxx)
TARGET_LINK_LIBRARIES(HierarchicalBoxPipeline vtkRendering vtkIO)
