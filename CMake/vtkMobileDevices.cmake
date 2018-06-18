# some specific defaults for Android to give folks
# a reasonable starting point
if (ANDROID OR APPLE_IOS)
  set(CMAKE_REQUIRE_LARGE_FILE_SUPPORT 2 CACHE STRING "Result from TRY_RUN" FORCE)
  set(CMAKE_REQUIRE_LARGE_FILE_SUPPORT__TRYRUN_OUTPUT "" CACHE STRING "Output from TRY_RUN" FORCE)
  set(KWSYS_LFS_WORKS 2 CACHE STRING "Result from TRY_RUN" FORCE)
  set(KWSYS_LFS_WORKS__TRYRUN_OUTPUT "" CACHE STRING "Output from TRY_RUN" FORCE)

  if (APPLE_IOS)
    unset(OPENGL_INCLUDE_DIR CACHE)
    find_path(OPENGL_INCLUDE_DIR ES3/gl.h
              ${CMAKE_OSX_SYSROOT}/System/Library/Frameworks/OpenGLES.framework/Headers
              ${_OPENGL_INCLUDE_DIR})
    find_library(OPENGL_gl_LIBRARY
       NAMES OpenGLES
       PATHS
         ${CMAKE_OSX_SYSROOT}/System/Library/Frameworks
         ${_OPENGL_LIB_PATH}
       )
  elseif (ANDROID)
    unset(OPENGL_INCLUDE_DIR CACHE)
    unset(OPENGL_gl_LIBRARY CACHE)
    unset(OPENGL_egl_LIBRARY CACHE)

    find_path(OPENGL_INCLUDE_DIR GLES3/gl3.h)
    find_library(OPENGL_gl_LIBRARY NAMES GLESv3)
    find_library(OPENGL_egl_LIBRARY NAMES EGL)
  endif()
else()
  # Choose static or shared libraries.
  option(BUILD_SHARED_LIBS "Build VTK with shared libraries." ON)
endif()
