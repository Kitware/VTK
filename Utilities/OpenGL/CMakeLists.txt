include(vtkOpenGLOptions)

if (OpenGL_GL_PREFERENCE STREQUAL "LEGACY")
  set(use_legacy ON)
else ()
  if (POLICY CMP0072)
    cmake_policy(GET CMP0072 opengl_policy)
    if (opengl_policy STREQUAL "NEW")
      set(use_legacy OFF)
    endif ()
  else ()
    # Without the policy, only legacy is availale.
    set(use_legacy ON)
  endif ()
endif ()

set(opengl_components)
set(opengl_optional_components)
if (VTK_USE_X)
  # XXX(nvidia): It seems that on some Ubuntu installations with manually
  # installed nVidia drivers, GLX is not found. Investigation is necessary.

  if (NOT use_legacy)
    list(APPEND opengl_optional_components GLX)
    list(APPEND opengl_targets OpenGL::GLX)
  endif ()
endif ()
if (VTK_OPENGL_USE_GLES)
  list(APPEND opengl_components GLES3)
  list(APPEND opengl_targets OpenGL::GLES3)
else ()
  list(APPEND opengl_components OpenGL)
  if (WIN32)
    list(APPEND opengl_targets OpenGL::GL)
  else ()
    if (use_legacy)
      list(APPEND opengl_targets OpenGL::GL)
    else ()
      list(APPEND opengl_targets OpenGL::OpenGL)
    endif ()
  endif ()
endif ()
if (VTK_OPENGL_HAS_EGL)
  list(APPEND opengl_components EGL)
  list(APPEND opengl_targets OpenGL::EGL)
endif ()

if (VTK_OPENGL_HAS_OSMESA AND NOT VTK_CAN_DO_ONSCREEN)
  vtk_module_third_party_external(
    PACKAGE OSMesa
    TARGETS OSMesa::OSMesa)
else ()
  cmake_policy(PUSH)
  if (POLICY CMP0072)
    cmake_policy(SET CMP0072 NEW) # prefer GLVND
  endif ()

  vtk_module_third_party_external(
    PACKAGE             OpenGL
    COMPONENTS          ${opengl_components}
    OPTIONAL_COMPONENTS ${opengl_optional_components}
    TARGETS             ${opengl_targets}
    INCLUDE_DIRS        OPENGL_INCLUDE_DIR
    LIBRARIES           OPENGL_LIBRARIES)

  cmake_policy(POP)

  if (APPLE)
    vtk_module_definitions(VTK::opengl
      INTERFACE
        GL_SILENCE_DEPRECATION)
  endif ()

  if (VTK_USE_OPENGL_DELAYED_LOAD)
    vtk_module_link(VTK::opengl
      INTERFACE
        delayimp.lib)
    vtk_module_link_options(VTK::opengl
      INTERFACE
        "/DELAYLOAD:opengl32.dll")
  endif ()
endif ()
