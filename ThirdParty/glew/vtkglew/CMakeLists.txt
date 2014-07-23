PROJECT(VTKGLEW)

find_package(OpenGL REQUIRED)

INCLUDE_DIRECTORIES(${VTKGLEW_SOURCE_DIR} ${VTKGLEW_SOURCE_DIR}/include)

# library object files common to compression and decompression
SET(common_SRCS src/glew.c)

set_source_files_properties(${common_SRCS}
    PROPERTIES
      COMPILE_DEFINITIONS "GLEW_NO_GLU")

VTK_ADD_LIBRARY(vtkglew ${common_SRCS})
target_link_libraries(vtkglew
  LINK_PUBLIC  ${OPENGL_LIBRARIES})
