# Logic to figure out what system libraries will be used by rendering, and
# whether VTK can use OSMesa for rendering.
set(VTK_USE_X_DEFAULT OFF)

if(APPLE AND NOT APPLE_IOS)
  option(VTK_USE_COCOA "Use Cocoa for VTK render windows" ON)
  option(VTK_USE_CARBON "Use Carbon for VTK render windows (deprecated)" OFF)
  mark_as_advanced(VTK_USE_COCOA VTK_USE_CARBON)
  if(VTK_USE_COCOA AND VTK_USE_CARBON)
    message(FATAL_ERROR "You can't enable Carbon and Cocoa - you must choose one.")
  endif()
elseif(UNIX AND NOT ANDROID AND NOT APPLE_IOS)
  set(VTK_USE_X_DEFAULT ON)
endif()

option(VTK_USE_X "Use X for VTK render windows" ${VTK_USE_X_DEFAULT})

# OSMesa logic for offscreen mesa rendering.
option(VTK_OPENGL_HAS_OSMESA
  "The OpenGL library being used supports off screen Mesa calls" OFF)
option(VTK_USE_OFFSCREEN "Use off screen calls by default" OFF)
unset(VTK_CAN_DO_OFFSCREEN)
if(VTK_OPENGL_HAS_OSMESA OR WIN32)
  set(VTK_CAN_DO_OFFSCREEN 1)
endif()
if(VTK_USE_X OR VTK_USE_CARBON OR VTK_USE_COCOA OR WIN32 OR ANDROID OR APPLE_IOS)
  set(VTK_USE_OSMESA FALSE)
else()
  set(VTK_USE_OSMESA TRUE)
endif()

mark_as_advanced(VTK_USE_X VTK_OPENGL_HAS_OSMESA VTK_USE_OFFSCREEN)
