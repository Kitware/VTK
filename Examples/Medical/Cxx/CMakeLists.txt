PROJECT (Medical)

INCLUDE (${CMAKE_ROOT}/Modules/FindVTK.cmake)
IF (USE_VTK_FILE)
  INCLUDE(${USE_VTK_FILE})
ENDIF (USE_VTK_FILE)

ADD_EXECUTABLE(Medical1 Medical1.cxx)
ADD_EXECUTABLE(Medical2 Medical2.cxx)
ADD_EXECUTABLE(Medical3 Medical3.cxx)

TARGET_LINK_LIBRARIES(Medical1 vtkRendering vtkIO)
TARGET_LINK_LIBRARIES(Medical2 vtkRendering vtkIO)
TARGET_LINK_LIBRARIES(Medical3 vtkRendering vtkIO)
