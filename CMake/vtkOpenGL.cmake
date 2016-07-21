# Logic to figure out what system libraries will be used by rendering, and
# whether VTK can use OSMesa for rendering.
set(VTK_USE_X_DEFAULT OFF)

if(APPLE AND NOT APPLE_IOS)
  option(VTK_USE_COCOA "Use Cocoa for VTK render windows" ON)
  mark_as_advanced(VTK_USE_COCOA)

  # VTK_USE_CARBON was deprecated for several releases, then removed in VTK 7.
  if (VTK_USE_CARBON)
    message(FATAL_ERROR "Carbon support has been removed, but it appears that it was requested. If you require Carbon support, use VTK 6.x.  Otherwise, turn off the VTK_USE_CARBON option.")
  endif ()
elseif(UNIX AND NOT ANDROID AND NOT APPLE_IOS)
  set(VTK_USE_X_DEFAULT ON)
endif()

option(VTK_USE_X "Use X for VTK render windows" ${VTK_USE_X_DEFAULT})

# OSMesa logic for offscreen mesa rendering.
option(VTK_OPENGL_HAS_OSMESA
  "The OpenGL library being used supports off screen Mesa calls" OFF)

# EGL offscreen rendering
option(VTK_USE_OFFSCREEN_EGL
  "Use EGL for OpenGL client API for offscreen rendering." OFF)

set(VTK_EGL_DEVICE_INDEX 0 CACHE STRING
  "Index of the EGL device (graphics card) to use.")

if (VTK_USE_OFFSCREEN_EGL AND VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  message(FATAL_ERROR "You can use VTK_USE_OFFSCREEN_EGL only for OpenGL2")
endif()
if (VTK_USE_OFFSCREEN_EGL AND ANDROID)
  message(FATAL_ERROR "You cannot use VTK_USE_OFFSCREEN_EGL on the ANDROID platform")
endif()

option(VTK_USE_OFFSCREEN "Use off screen calls by default" OFF)

unset(VTK_CAN_DO_OFFSCREEN)
unset(VTK_CAN_DO_ONSCREEN)

if(VTK_OPENGL_HAS_OSMESA OR WIN32 OR VTK_USE_OFFSCREEN_EGL)
  set(VTK_CAN_DO_OFFSCREEN 1)
endif()

if(VTK_USE_X OR VTK_USE_COCOA OR WIN32 OR ANDROID OR APPLE_IOS)
  set(VTK_USE_OSMESA ${VTK_OPENGL_HAS_OSMESA})
  if (VTK_USE_OFFSCREEN_EGL)
    message(FATAL_ERROR "VTK_USE_OFFSCREEN_EGL set together with one of ("
      "VTK_USE_X, VTK_USE_COCOA, WIN32, ANDROID OR APPLE_IOS). "
      "You cannot use both offscreen and one of the listed windowing systems.")
  endif()
  set(VTK_CAN_DO_ONSCREEN 1)
elseif(VTK_USE_OFFSCREEN_EGL)
  set(VTK_USE_OSMESA FALSE)
else()
  set(VTK_USE_OSMESA ${VTK_OPENGL_HAS_OSMESA})
endif()

mark_as_advanced(VTK_USE_X VTK_OPENGL_HAS_OSMESA VTK_USE_OFFSCREEN_EGL
  VTK_USE_OFFSCREEN VTK_EGL_DEVICE_INDEX)

if(VTK_USE_OSMESA)
  find_package(OSMesa REQUIRED)
  include_directories(SYSTEM ${OSMESA_INCLUDE_DIR})
endif()

if(VTK_USE_OFFSCREEN_EGL)
  find_package(EGL REQUIRED)
  include_directories(SYSTEM ${EGL_INCLUDE_DIR})
endif()

if(VTK_CAN_DO_ONSCREEN)
  # OpenGL libraries are explicity needed if windowing system-based API is being
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

# Function to link a VTK target to the necessary OpenGL libraries.
function(vtk_opengl_link target)
  if(VTK_USE_OSMESA)
    vtk_module_link_libraries(${target} LINK_PRIVATE ${OSMESA_LIBRARY})
  endif()
  if(VTK_USE_OFFSCREEN_EGL)
    vtk_module_link_libraries(${target} LINK_PRIVATE ${EGL_LIBRARIES})
  endif()
  if(VTK_CAN_DO_ONSCREEN)
    vtk_module_link_libraries(${target} LINK_PRIVATE ${OPENGL_LIBRARIES})
  endif()
endfunction()
