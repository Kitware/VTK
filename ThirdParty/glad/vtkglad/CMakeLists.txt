set(headers
  "include/KHR/khrplatform.h")
set(sources)

list(APPEND headers
  "include/glad/vtk_gl_mangle.h"
  "include/glad/gl.h")
list(APPEND sources
  "src/gl.c")

if (WIN32)
  list(APPEND sources
    "src/wgl.c")
  list(APPEND headers
    "include/glad/vtk_wgl_mangle.h"
    "include/glad/wgl.h")
endif ()
if (VTK_OPENGL_HAS_EGL)
  list(APPEND sources "src/egl.c")
  list(APPEND headers
    "include/glad/vtk_egl_mangle.h"
    "include/glad/egl.h"
    "include/EGL/eglplatform.h")
endif ()
if (VTK_USE_X)
  list(APPEND sources
    "src/glx.c")
  list(APPEND headers
    "include/glad/vtk_glx_mangle.h"
    "include/glad/glx.h")
endif ()

vtk_module_add_module(VTK::glad
  HEADER_DIRECTORIES
  SOURCES ${sources}
  HEADERS ${headers}
  HEADERS_SUBDIR "vtkglad")
target_include_directories(glad
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
if (BUILD_SHARED_LIBS)
  target_compile_definitions(glad
    PUBLIC
      "GLAD_API_CALL_EXPORT")
  set_target_properties(glad
    PROPERTIES
      DEFINE_SYMBOL "GLAD_API_CALL_EXPORT_BUILD")
endif ()

if (VTK_USE_X)
  find_package(X11 REQUIRED)
  vtk_module_find_package(PACKAGE X11)
  vtk_module_link(VTK::glad PRIVATE X11::X11) # needed for `XOpenDisplay` in glx.c
endif ()

if (APPLE)
  vtk_module_definitions(VTK::glad
    INTERFACE
      GL_SILENCE_DEPRECATION)
endif ()

if (WIN32)
  vtk_module_link(VTK::glad PUBLIC opengl32)
  if (VTK_USE_OPENGL_DELAYED_LOAD)
    vtk_module_link(VTK::glad
      INTERFACE
        delayimp.lib)
    vtk_module_link_options(VTK::glad
      INTERFACE
        "/DELAYLOAD:opengl32.dll")
  endif ()
elseif (VTK_OPENGL_USE_GLES)
  vtk_module_find_package(PACKAGE OpenGL COMPONENTS GLES3)
  vtk_module_link(VTK::glad PUBLIC OpenGL::GLES3)
endif ()
