include(CMakeDependentOption)

# Logic to figure out what system libraries will be used by rendering, and
# whether VTK can use OSMesa for rendering.
set(default_use_x OFF)

# For each platform specific API, we define VTK_USE_<API> options.
if(APPLE AND NOT APPLE_IOS)
  option(VTK_USE_COCOA "Use Cocoa for VTK render windows" ON)
  mark_as_advanced(VTK_USE_COCOA)

  # VTK_USE_CARBON was deprecated for several releases, then removed in VTK 7.
  if (VTK_USE_CARBON)
    message(FATAL_ERROR "Carbon support has been removed, but it appears that it was requested. If you require Carbon support, use VTK 6.x.  Otherwise, turn off the VTK_USE_CARBON option.")
  endif ()
elseif(UNIX AND NOT ANDROID AND NOT APPLE_IOS AND NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
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
# EGL variables
#-----------------------------------------------------------------------------
# OpenGL implementation supports EGL for creating offscreen context.
set(default_has_egl OFF)
if(DEFINED VTK_USE_OFFSCREEN_EGL AND VTK_USE_OFFSCREEN_EGL)
  message(DEPRECATION "`VTK_USE_OFFSCREEN_EGL` cache variable is replaced by "
    "`VTK_OPENGL_HAS_EGL`. Please use it instead. The new name "
    "better reflects the variable's purpose.")
  set(default_has_egl ${VTK_USE_OFFSCREEN_EGL})
endif()

option(VTK_OPENGL_HAS_EGL "The OpenGL library being used supports EGL" ${default_has_egl})
mark_as_advanced(VTK_OPENGL_HAS_EGL)

set(default_egl_device_index 0)
if(DEFINED VTK_EGL_DEVICE_INDEX AND NOT VTK_EGL_DEVICE_INDEX EQUAL 0)
  message(DEPRECATION "`VTK_EGL_DEVICE_INDEX` cache variable is replaced by "
    "`VTK_DEFAULT_EGL_DEVICE_INDEX`. Please use it instead. The new name "
    "better reflects the variable's purpose.")
  set(default_egl_device_index ${VTK_EGL_DEVICE_INDEX})
endif()

set(VTK_DEFAULT_EGL_DEVICE_INDEX "${default_egl_device_index}" CACHE STRING
  "EGL device (graphics card) index to use by default for EGL render windows.")
mark_as_advanced(VTK_DEFAULT_EGL_DEVICE_INDEX)

# Some sanity checks for use of EGL.
if (VTK_OPENGL_HAS_EGL AND ANDROID)
  message(FATAL_ERROR "You cannot use VTK_OPENGL_HAS_EGL on the ANDROID platform")
endif()

#-----------------------------------------------------------------------------
# Irrespective of support for offscreen API, VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN
# lets the user select the default state for the  `Offscreen` flag on the
# vtkRenderWindow when it is instantiated (formerly VTK_USE_OFFSCREEN).
set(default_use_offscreen OFF)
if(DEFINED VTK_USE_OFFSCREEN AND VTK_USE_OFFSCREEN)
  message(DEPRECATION "`VTK_USE_OFFSCREEN` cache variable is replaced by "
    "`VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN`. Please use it instead. The new name "
    "better reflects the variable's purpose.")
  set(default_use_offscreen ${VTK_USE_OFFSCREEN})
endif()
option(VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN "Use offscreen calls by default" ${default_use_offscreen})
mark_as_advanced(VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN)

#-----------------------------------------------------------------------------
set(VTK_CAN_DO_OFFSCREEN FALSE)
set(VTK_CAN_DO_ONSCREEN FALSE)
set(VTK_CAN_DO_HEADLESS FALSE)

if(WIN32 OR VTK_OPENGL_HAS_OSMESA OR VTK_OPENGL_HAS_EGL)
  set(VTK_CAN_DO_OFFSCREEN TRUE)
endif()
if(WIN32 OR VTK_USE_COCOA OR VTK_USE_X)
  set(VTK_CAN_DO_ONSCREEN TRUE)
endif()

if(VTK_OPENGL_HAS_OSMESA OR VTK_OPENGL_HAS_EGL)
  set(VTK_CAN_DO_HEADLESS TRUE)
endif()

#-----------------------------------------------------------------------------
if(NOT APPLE_IOS AND NOT ANDROID AND NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  # For builds where we can support both on-screen and headless rendering, the default
  # is to create an on-screen render window. Setting this option to ON will change the default
  # to create an headless render window by default instead.
  cmake_dependent_option(
    VTK_DEFAULT_RENDER_WINDOW_HEADLESS
    "Enable to create the headless render window when `vtkRenderWindow` is instantiated."
    OFF "VTK_CAN_DO_ONSCREEN;VTK_CAN_DO_HEADLESS" OFF)
else()
  set(VTK_DEFAULT_RENDER_WINDOW_HEADLESS OFF)
endif()

#-----------------------------------------------------------------------------
# The following assumes shared dispatch between onscreen and offscreen GL
# implementations.

if(VTK_OPENGL_HAS_OSMESA)
  find_package(OSMesa REQUIRED)
  include_directories(SYSTEM ${OSMESA_INCLUDE_DIR})
endif()

if(VTK_OPENGL_HAS_EGL)
  find_package(EGL REQUIRED)
endif()

if(VTK_CAN_DO_ONSCREEN)
  # OpenGL libraries are explicitly needed if windowing system-based API is being
  # used. Otherwise, if only doing OFFSCREEN, the GL API is provided by the
  # offscreen library be it EGL or OSMESA.
  find_package(OpenGL REQUIRED)
  include_directories(SYSTEM ${OPENGL_INCLUDE_DIR})
  if(APPLE)
    # Remove the deprecated AGL framework found by FindOpenGL.cmake
    # (this is only required for CMake 3.4.1 and earlier)
    set(_new_libs)
    foreach(_lib ${OPENGL_LIBRARIES})
      get_filename_component(_name "${_lib}" NAME)
      string(TOLOWER "${_name}" _name)
      if(NOT "${_name}" STREQUAL "agl.framework")
        list(APPEND _new_libs ${_lib})
      endif()
    endforeach()
    set(OPENGL_LIBRARIES ${_new_libs})
    unset(_new_libs)
    unset(_name)
  endif()
endif()

# windows  opengl delayed loading option
if(WIN32)
  option(VTK_USE_OPENGL_DELAYED_LOAD "Use delayed loading for the opengl dll" FALSE)
  mark_as_advanced(VTK_USE_OPENGL_DELAYED_LOAD)
endif()

# Function to link a VTK target to the necessary OpenGL libraries.
function(vtk_opengl_link target)
  if(VTK_OPENGL_HAS_OSMESA)
    vtk_module_link_libraries(${target} LINK_PRIVATE ${OSMESA_LIBRARY})
  endif()
  if(VTK_OPENGL_HAS_EGL)
    vtk_module_link_libraries(${target} LINK_PRIVATE EGL::EGL)
  endif()
  if(VTK_CAN_DO_ONSCREEN)
    vtk_module_link_libraries(${target} LINK_PRIVATE ${OPENGL_LIBRARIES})
  endif()
  if (VTK_USE_OPENGL_DELAYED_LOAD)
    vtk_module_link_libraries(${target} LINK_PRIVATE delayimp.lib)
    set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS "/DELAYLOAD:opengl32.dll")
  endif()
endfunction()
