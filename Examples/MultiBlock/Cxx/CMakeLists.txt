PROJECT (MultiBlock)

INCLUDE (${CMAKE_ROOT}/Modules/FindVTK.cmake)
IF (USE_VTK_FILE)
  INCLUDE(${USE_VTK_FILE})
ENDIF (USE_VTK_FILE)

ADD_EXECUTABLE(MultiBlock MultiBlock.cxx)
TARGET_LINK_LIBRARIES(MultiBlock vtkRendering vtkIO)
