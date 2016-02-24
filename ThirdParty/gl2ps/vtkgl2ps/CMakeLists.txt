project(VTKGL2PS)

# Dependency mask
include_regular_expression(".*")

# Depend on OpenGL
include(vtkOpenGL)

# Include paths
include_directories(
  ${VTKGL2PS_SOURCE_DIR}
)

# Source files
set(GL2PS_srcs
  gl2ps.c
)

# Define the library (and install it)
vtk_add_library(vtkgl2ps ${GL2PS_srcs})

# Setup definitions
if(WIN32)
  set_source_files_properties(${GL2PS_srcs}
    PROPERTIES
      COMPILE_DEFINITIONS "GL2PSDLL;GL2PSDLL_EXPORTS")
endif()

set_property(TARGET vtkgl2ps
   APPEND PROPERTY COMPILE_DEFINITIONS
   HAVE_ZLIB HAVE_PNG
)

# Linking
target_link_libraries(vtkgl2ps LINK_PRIVATE
  ${vtkzlib_LIBRARIES}
  ${vtkpng_LIBRARIES}
)
vtk_opengl_link(vtkgl2ps)

if(UNIX)
  target_link_libraries(vtkgl2ps LINK_PRIVATE m)
endif()

# Install headers
if(NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES
    ${VTKGL2PS_SOURCE_DIR}/gl2ps.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}/vtkgl2ps
    COMPONENT Development)
ENDIF()
