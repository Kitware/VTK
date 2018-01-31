set(sources
  src/glew.c)

set(headers
  include/GL/glew.h
  include/GL/glxew.h
  include/GL/vtk_glew_mangle.h
  include/GL/wglew.h)

vtk_add_library(vtkglew ${sources} ${headers})
if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES
    ${headers}
    DESTINATION "${VTK_INSTALL_INCLUDE_DIR}/vtkglew/include/GL"
    COMPONENT Development)
endif()
include(vtkOpenGL)
vtk_opengl_link(vtkglew)
target_compile_definitions(vtkglew
  PRIVATE
    GLEW_NO_GLU)

# GLEW with OSMesa only works on Linux systems right now.
if(VTK_OPENGL_HAS_OSMESA AND UNIX AND NOT APPLE)
  target_compile_definitions(vtkglew
    PRIVATE
      GLEW_OSMESA)
  target_link_libraries(vtkglew
    PRIVATE
      ${CMAKE_DL_LIBS})
endif()

if(VTK_OPENGL_HAS_EGL)
  target_compile_definitions(vtkglew
    PRIVATE
      GLEW_EGL)
endif()

if (BUILD_SHARED_LIBS)
  if (WIN32)
    target_compile_definitions(vtkglew
      PRIVATE
        GLEW_BUILD)
  endif ()
else ()
  target_compile_definitions(vtkglew
    PUBLIC
      GLEW_STATIC)
endif ()

target_include_directories(vtkglew
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include")
