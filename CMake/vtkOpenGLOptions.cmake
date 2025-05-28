#[==[.md
# vtkOpenGLOptions

This module provides options that control which OpenGL and Windowing system
libraries are used.

#]==]
include(CMakeDependentOption)

# For each platform specific API, we define VTK_USE_<API> options.
cmake_dependent_option(VTK_USE_COCOA "Use Cocoa for VTK render windows" ON
  "APPLE;NOT APPLE_IOS" OFF)
mark_as_advanced(VTK_USE_COCOA)

set(default_use_x OFF)
if(UNIX AND NOT ANDROID AND NOT APPLE AND NOT APPLE_IOS AND NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(default_use_x ON)
endif()
option(VTK_USE_X "Use X for VTK render windows" "${default_use_x}")
mark_as_advanced(VTK_USE_X)

cmake_dependent_option(VTK_USE_WIN32_OPENGL "Use Win32 APIs for VTK render windows" ON
  "WIN32" OFF)
mark_as_advanced(VTK_USE_WIN32_OPENGL)

set(default_use_sdl2 OFF)
# VTK_DEPRECATED_IN_9_4_0() Remove option when vtkSDL2OpenGLRenderWindow and vtkSDL2WebGPURenderWindow are removed.
option(VTK_USE_SDL2 "Add SDL2 classes to VTK. This option will soon be removed" "${default_use_sdl2}")
mark_as_advanced(VTK_USE_SDL2)
if (VTK_USE_SDL2)
  message(WARNING "You are using a soon to be deprecated flag. The VTK_USE_SDL2 option is marked for deprecation in VTK 9.4!")
endif ()

# For optional APIs that could be available for the OpenGL implementation
# being used, we define VTK_OPENGL_HAS_<feature> options. These are not to be
# treated as mutually exclusive.

#-----------------------------------------------------------------------------
# GLES variables
#-----------------------------------------------------------------------------

set(default_has_egl OFF)
set(default_use_gles OFF)
if (ANDROID)
  set(default_has_egl ON)
  set(default_use_gles ON)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(default_use_gles ON)
elseif (UNIX AND NOT APPLE)
  set(default_has_egl ON)
endif ()
# OpenGLES implementation.
option(VTK_OPENGL_USE_GLES "Use the OpenGL ES API" "${default_use_gles}")
mark_as_advanced(VTK_OPENGL_USE_GLES)

#-----------------------------------------------------------------------------
# EGL variables
#-----------------------------------------------------------------------------
# Whether VTK should attempt to use EGL for creating offscreen context.
option(VTK_OPENGL_HAS_EGL "Enable EGL support for creating GPU accelerated offscreen context" "${default_has_egl}")
mark_as_advanced(VTK_OPENGL_HAS_EGL)
if (VTK_OPENGL_HAS_EGL AND APPLE)
  message(FATAL_ERROR "VTK_OPENGL_HAS_EGL is ON, but APPLE platform does not support EGL!")
endif ()
set(VTK_DEFAULT_EGL_DEVICE_INDEX "0" CACHE STRING
  "EGL device (graphics card) index to use by default for EGL render windows.")
mark_as_advanced(VTK_DEFAULT_EGL_DEVICE_INDEX)

#-----------------------------------------------------------------------------
# Irrespective of support for offscreen API, VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN
# lets the user select the default state for the  `Offscreen` flag on the
# vtkRenderWindow when it is instantiated (formerly VTK_USE_OFFSCREEN).
option(VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN "Use offscreen render window by default" OFF)
mark_as_advanced(VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN)

#-----------------------------------------------------------------------------
set(vtk_can_do_offscreen FALSE)
set(vtk_can_do_onscreen FALSE)
# VTK OSMesa support is always built on major desktop platforms because it's far cheaper and simpler
# to just build software-only support rather than making `vtkOpenGLRenderWindow` handle situations when
# neither the hardware accelerated on/offscreen backends, nor the software-only backends are available.
set(vtk_can_do_headless TRUE)

if (VTK_USE_WIN32_OPENGL OR VTK_OPENGL_HAS_EGL OR VTK_USE_SDL2)
  set(vtk_can_do_offscreen TRUE)
endif ()
if (VTK_USE_WIN32_OPENGL OR VTK_USE_COCOA OR VTK_USE_X OR VTK_USE_SDL2) # XXX: See error message below.
  set(vtk_can_do_onscreen TRUE)
endif ()

# iOS does not use EGL
if (APPLE_IOS)
  set(vtk_can_do_offscreen TRUE)
  set(vtk_can_do_onscreen TRUE)
  set(vtk_can_do_headless FALSE)
endif ()

if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(vtk_can_do_headless FALSE)
  # VTK_DEPRECATED_IN_9_4_0() Unconditionally set both variables to TRUE after VTK_USE_SDL2 is removed.
  if (NOT VTK_USE_SDL2)
    set(vtk_can_do_onscreen TRUE)
    set(vtk_can_do_offscreen TRUE)
  endif ()
endif ()

cmake_dependent_option(
  VTK_USE_OPENGL_DELAYED_LOAD
  "Use delay loading for OpenGL"
  OFF "VTK_USE_WIN32_OPENGL;COMMAND target_link_options" OFF)
mark_as_advanced(VTK_USE_OPENGL_DELAYED_LOAD)

#-----------------------------------------------------------------------------
# For builds where we can support both on-screen and headless rendering, the default
# is to create an on-screen render window. Setting this option to ON will change the default
# to create an headless render window by default instead.
cmake_dependent_option(
  VTK_DEFAULT_RENDER_WINDOW_HEADLESS
  "Enable to create the headless render window when `vtkRenderWindow` is instantiated."
  OFF
  "vtk_can_do_onscreen;vtk_can_do_headless" OFF)
