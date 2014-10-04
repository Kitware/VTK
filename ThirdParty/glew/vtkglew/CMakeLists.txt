project(VTKGLEW)

find_package(OpenGL REQUIRED)
include(vtkOpenGLMesaLogic)

include_directories(${VTKGLEW_SOURCE_DIR} ${VTKGLEW_SOURCE_DIR}/include)

# library object files common to compression and decompression
set(common_SRCS src/glew.c)

set_source_files_properties(${common_SRCS}
    PROPERTIES
      COMPILE_DEFINITIONS "GLEW_NO_GLU")

# GLEW with OSMesa only works on Linux systems right now.
if(VTK_USE_OSMESA AND UNIX AND NOT APPLE)
  add_definitions(-DGLEW_OSMESA)
endif()

vtk_add_library(vtkglew ${common_SRCS})
target_link_libraries(vtkglew
  LINK_PUBLIC ${OPENGL_LIBRARIES})
if(VTK_USE_OSMESA AND UNIX AND NOT APPLE)
  target_link_libraries(vtkglew LINK_PRIVATE ${CMAKE_DL_LIBS})
endif()

if(NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(DIRECTORY ${VTKGLEW_SOURCE_DIR}/include
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}/vtkglew
    COMPONENT Development)
endif()
