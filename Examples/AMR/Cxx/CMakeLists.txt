PROJECT (AMR)

FIND_PACKAGE(VTK)
IF(NOT VTK_DIR)
  MESSAGE(FATAL_ERROR "Please set VTK_DIR.")
ENDIF(NOT VTK_DIR)
INCLUDE(${VTK_USE_FILE})

ADD_EXECUTABLE(HierarchicalBox HierarchicalBox.cxx)
TARGET_LINK_LIBRARIES(HierarchicalBox vtkRendering vtkIO)

ADD_EXECUTABLE(HierarchicalBoxPipeline HierarchicalBoxPipeline.cxx)
TARGET_LINK_LIBRARIES(HierarchicalBoxPipeline vtkRendering vtkIO)
