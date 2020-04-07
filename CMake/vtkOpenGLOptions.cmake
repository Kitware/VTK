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
option(VTK_USE_X "Use X for VTK render windows" ${default_use_x})
mark_as_advanced(VTK_USE_X)

# For optional APIs that could be available for the OpenGL implementation
# being used, we define VTK_OPENGL_HAS_<feature> options. These are not to be
# treated as mutually exclusive.

#-----------------------------------------------------------------------------
# OSMesa variables
#-----------------------------------------------------------------------------
# OpenGL implementation supports OSMesa for creating offscreen context.
option(VTK_OPENGL_HAS_OSMESA
  "The OpenGL library being used supports offscreen Mesa (OSMesa)" OFF)
mark_as_advanced(VTK_OPENGL_HAS_OSMESA)

#-----------------------------------------------------------------------------
# GLES variables
#-----------------------------------------------------------------------------

set(default_has_egl OFF)
if (ANDROID)
  set(VTK_OPENGL_USE_GLES ON CACHE INTERNAL "Use the OpenGL ES API")
  set(default_has_egl ON)
else ()
  # OpenGLES implementation.
  option(VTK_OPENGL_USE_GLES "Use the OpenGL ES API" OFF)
  mark_as_advanced(VTK_OPENGL_USE_GLES)
endif ()

#-----------------------------------------------------------------------------
# EGL variables
#-----------------------------------------------------------------------------
# OpenGL implementation supports EGL for creating offscreen context.
option(VTK_OPENGL_HAS_EGL "The OpenGL library being used supports EGL" "${default_has_egl}")
mark_as_advanced(VTK_OPENGL_HAS_EGL)

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
set(VTK_CAN_DO_OFFSCREEN FALSE)
set(VTK_CAN_DO_ONSCREEN FALSE)
set(VTK_CAN_DO_HEADLESS FALSE)

if(WIN32 OR VTK_OPENGL_HAS_OSMESA OR VTK_OPENGL_HAS_EGL)
  set(VTK_CAN_DO_OFFSCREEN TRUE)
endif()
if(WIN32 OR VTK_USE_COCOA OR VTK_USE_X) # XXX: See error message below.
  set(VTK_CAN_DO_ONSCREEN TRUE)
endif()

if(VTK_OPENGL_HAS_OSMESA OR VTK_OPENGL_HAS_EGL)
  set(VTK_CAN_DO_HEADLESS TRUE)
endif()

if(APPLE_IOS OR ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(VTK_CAN_DO_HEADLESS FALSE)
endif()

if (VTK_OPENGL_HAS_OSMESA AND VTK_CAN_DO_ONSCREEN)
  message(FATAL_ERROR
    "The `VTK_OPENGL_HAS_OSMESA` is ignored if any of the following is true: "
    "the target platform is Windows, `VTK_USE_COCOA` is `ON`, or `VTK_USE_X` "
    "is `ON`. OSMesa does not support on-screen rendering and VTK's OpenGL "
    "selection is at build time, so the current build configuration is not "
    "satisfiable.")
endif ()

cmake_dependent_option(
  VTK_USE_OPENGL_DELAYED_LOAD
  "Use delay loading for OpenGL"
  OFF "WIN32;COMMAND target_link_options" OFF)
mark_as_advanced(VTK_USE_OPENGL_DELAYED_LOAD)

#-----------------------------------------------------------------------------
# For builds where we can support both on-screen and headless rendering, the default
# is to create an on-screen render window. Setting this option to ON will change the default
# to create an headless render window by default instead.
cmake_dependent_option(
  VTK_DEFAULT_RENDER_WINDOW_HEADLESS
  "Enable to create the headless render window when `vtkRenderWindow` is instantiated."
  OFF "VTK_CAN_DO_ONSCREEN;VTK_CAN_DO_HEADLESS" OFF)
