PROJECT (finance)

INCLUDE (${CMAKE_ROOT}/Modules/FindVTK.cmake)
IF (USE_VTK_FILE)
  INCLUDE(${USE_VTK_FILE})
ENDIF (USE_VTK_FILE)

ADD_EXECUTABLE(finance finance.cxx)
TARGET_LINK_LIBRARIES(finance vtkRendering)
