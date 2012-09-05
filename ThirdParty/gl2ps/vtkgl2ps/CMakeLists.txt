project(VTKGL2PS)

# Dependency mask
include_regular_expression(".*")

# Depend on OpenGL
if(NOT VTK_OPENGL_HAS_OSMESA)
  find_package(OpenGL REQUIRED)
  set(GL2PS_GL_LIBRARIES ${OPENGL_LIBRARIES})
  set(GL2PS_GL_INCLUDE_DIR ${OPENGL_INCLUDE_DIR})
else()
  set(GL2PS_GL_LIBRARIES ${OSMESA_LIBRARY})
  set(GL2PS_GL_INCLUDE_DIR ${OSMESA_INCLUDE_DIR})
  if(OPENGL_gl_LIBRARY)
    list(APPEND GL2PS_GL_LIBRARIES ${OPENGL_gl_LIBRARY})
  endif()
endif()

# Need math on UNIX
if(UNIX)
  set(GL2PS_GL_LIBRARIES ${GL2PS_GL_LIBRARIES} m)
endif()

# Include paths
include_directories(
  ${VTKGL2PS_SOURCE_DIR}/
  ${GL2PS_GL_INCLUDE_DIR}
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
target_link_libraries(vtkgl2ps
  ${GL2PS_GL_LIBRARIES}
  ${vtkzlib_LIBRARIES}
  ${vtkpng_LIBRARIES}
)
if(UNIX)
  target_link_libraries(vtkgl2ps m)
endif()

# Install headers
if(NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES
    ${VTKGL2PS_SOURCE_DIR}/gl2ps.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}/vtkgl2ps/include
    COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
