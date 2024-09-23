# some specific defaults for Android to give folks
# a reasonable starting point
if (ANDROID OR APPLE_IOS)
  set(VTK_REQUIRE_LARGE_FILE_SUPPORT 2 CACHE STRING "Result from TRY_RUN" FORCE)
  set(VTK_REQUIRE_LARGE_FILE_SUPPORT__TRYRUN_OUTPUT "" CACHE STRING "Output from TRY_RUN" FORCE)
  set(XDMF_REQUIRE_LARGE_FILE_SUPPORT 2 CACHE STRING "Result from TRY_RUN" FORCE)
  set(XDMF_REQUIRE_LARGE_FILE_SUPPORT__TRYRUN_OUTPUT "" CACHE STRING "Output from TRY_RUN" FORCE)
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
    unset(OPENGL_gles3_LIBRARY CACHE)
    unset(OPENGL_egl_LIBRARY CACHE)

    set(_ANDROID_INC_PATH ${CMAKE_ANDROID_NDK}/sysroot/usr/include)
    set(_ANDROID_LIB_PATH ${CMAKE_ANDROID_NDK}/platforms/android-${CMAKE_SYSTEM_VERSION}/arch-${CMAKE_ANDROID_ARCH}/usr/lib)

    find_path(OPENGL_INCLUDE_DIR GLES3/gl3.h ${_ANDROID_INC_PATH})
    find_library(OPENGL_gles3_LIBRARY NAMES GLESv3 PATHS ${_ANDROID_LIB_PATH})
    find_library(OPENGL_egl_LIBRARY NAMES EGL PATHS ${_ANDROID_LIB_PATH})
  endif()
endif()
