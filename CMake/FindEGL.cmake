#.rst:
# FindEGL
# -------
#
# Find the EGL library.
#
# Imported Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines the following :prop_tgt:`IMPORTED` targets:
#
# ``EGL::EGL``
#   The EGL library, if found.
#
# ``EGL::OpenGL``
#   The OpenGL library, if found.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module will set the following variables in your project:
#
# ``EGL_FOUND``
#   System has the EGL library.
# ``EGL_INCLUDE_DIR``
#   The EGL include directory.
# ``EGL_LIBRARY``
#   The libEGL library.
# ``EGL_LIBRARIES``
#   All EGL related libraries, including ``EGL_LIBRARY``.
#
# Hints
# ^^^^^
#
# Set `EGL_ROOT_DIR` to the root directory of an EGL installation.
find_path(EGL_INCLUDE_DIR
  NAMES
    EGL/egl.h
  PATHS
    ${EGL_ROOT_DIR}/include
    /usr/local/include
    /usr/include)

find_library(EGL_LIBRARY
  NAMES
    EGL
  PATHS
    ${EGL_ROOT_DIR}/lib
    /usr/local/lib
    /usr/lib)

find_library(EGL_opengl_LIBRARY
  NAMES
   OpenGL
  PATHS
    ${EGL_ROOT_DIR}/lib
    /usr/local/lib
    /usr/lib)

set(EGL_LIBRARIES ${EGL_LIBRARY} ${EGL_opengl_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  EGL DEFAULT_MSG
  EGL_LIBRARY EGL_opengl_LIBRARY EGL_INCLUDE_DIR)
mark_as_advanced(EGL_ROOT_DIR EGL_INCLUDE_DIR EGL_LIBRARY EGL_opengl_LIBRARY)

if(EGL_FOUND)
  if(NOT TARGET EGL::OpenGL)
    add_library(EGL::OpenGL UNKNOWN IMPORTED)
    set_target_properties(EGL::OpenGL PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${EGL_INCLUDE_DIR}")
    set_target_properties(EGL::OpenGL PROPERTIES
      IMPORTED_LINK_INTERFACE_LANGUAGES "C"
      IMPORTED_LOCATION "${EGL_opengl_LIBRARY}")
  endif()

  if(NOT TARGET EGL::EGL)
    add_library(EGL::EGL UNKNOWN IMPORTED)
    set_target_properties(EGL::EGL PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${EGL_INCLUDE_DIR}")
    set_target_properties(EGL::EGL PROPERTIES
      IMPORTED_LINK_INTERFACE_LANGUAGES "C"
      INTERFACE_LINK_LIBRARIES "EGL::OpenGL"
      IMPORTED_LOCATION "${EGL_LIBRARY}")
  endif()
endif()
