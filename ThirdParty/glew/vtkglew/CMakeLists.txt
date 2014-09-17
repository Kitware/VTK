project(VTKGLEW)

find_package(OpenGL REQUIRED)

include_directories(${VTKGLEW_SOURCE_DIR} ${VTKGLEW_SOURCE_DIR}/include)

# library object files common to compression and decompression
set(common_SRCS src/glew.c)

set_source_files_properties(${common_SRCS}
    PROPERTIES
      COMPILE_DEFINITIONS "GLEW_NO_GLU")

vtk_add_library(vtkglew ${common_SRCS})
target_link_libraries(vtkglew
  LINK_PUBLIC ${OPENGL_LIBRARIES})

if(NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(DIRECTORY ${VTKGLEW_SOURCE_DIR}/include
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}/vtkglew
    COMPONENT Development)
endif()
