##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

#-----------------------------------------------------------------------------
# find OpenGL and supporting libraries provided to make using Viskores easier
# for consumers.
#
# viskores_find_gl(
#   REQUIRED [GL|GLUT|GLEW]
#   OPTIONAL [GL|GLUT|GLEW]
#   QUIET [ON/OFF]
#   )
#
# Imports everything as imported modules with the following names:
#  - GLUT::GLUT
#  - GLEW::GLEW
#  - OpenGL::GL
# For OpenGL Will also provide the more explicit targets of:
#  - OpenGL::OpenGL
#  - OpenGL::GLU
#  - OpenGL::GLX
#  - OpenGL::EGL
function(viskores_find_gl)
  set(oneValueArgs QUIET)
  set(multiValueArgs REQUIRED OPTIONAL)
  cmake_parse_arguments(find_gl
    "${options}" "${oneValueArgs}" "${multiValueArgs}"
    ${ARGN}
    )

  set(QUIETLY )
  if(find_gl_QUIET)
    set(QUIETLY "QUIET")
  endif()

  foreach(item ${find_gl_REQUIRED})
    set(${item}_REQUIRED "REQUIRED")
    set(DO_${item}_FIND TRUE)
  endforeach()

  foreach(item ${find_gl_OPTIONAL})
    set(DO_${item}_FIND TRUE)
  endforeach()

  #Find GL
  if(DO_GL_FIND AND NOT TARGET OpenGL::GL)
    find_package(OpenGL ${GL_REQUIRED} ${QUIETLY} MODULE)
  endif()

  #Find GLEW
  if(DO_GLEW_FIND AND NOT TARGET GLEW::GLEW)
    find_package(GLEW ${GLEW_REQUIRED} ${QUIETLY})
  endif()

  if(DO_GLUT_FIND AND NOT TARGET GLUT::GLUT)
    find_package(GLUT ${GLUT_REQUIRED} ${QUIETLY})

    if(APPLE AND CMAKE_VERSION VERSION_LESS 3.19.2)
      get_target_property(lib_path GLUT::GLUT IMPORTED_LOCATION)
      if(EXISTS "${lib_path}.tbd")
        set_target_properties(GLUT::GLUT PROPERTIES
          IMPORTED_LOCATION "${lib_path}.tbd")
      endif()

      get_target_property(lib_path GLUT::Cocoa IMPORTED_LOCATION)
      if(EXISTS "${lib_path}.tbd")
        set_target_properties(GLUT::Cocoa PROPERTIES
          IMPORTED_LOCATION "${lib_path}.tbd")
      endif()
    endif()
  endif()

endfunction()

#-----------------------------------------------------------------------------
if(Viskores_ENABLE_GL_CONTEXT OR
   Viskores_ENABLE_OSMESA_CONTEXT OR
   Viskores_ENABLE_EGL_CONTEXT
   )
  viskores_find_gl(REQUIRED GL GLEW
               OPTIONAL
               QUIET)
endif()

#-----------------------------------------------------------------------------
if(Viskores_ENABLE_RENDERING AND NOT TARGET viskores_rendering_gl_context)
  add_library(viskores_rendering_gl_context INTERFACE)

  if(NOT Viskores_INSTALL_ONLY_LIBRARIES)
    viskores_install_targets(TARGETS viskores_rendering_gl_context)
  endif()
endif()

#-----------------------------------------------------------------------------
if(Viskores_ENABLE_GL_CONTEXT)
  if(TARGET OpenGL::GLX)
    target_link_libraries(viskores_rendering_gl_context
                          INTERFACE OpenGL::OpenGL OpenGL::GLX GLEW::GLEW)
  elseif(TARGET OpenGL::GL)
    target_link_libraries(viskores_rendering_gl_context
                          INTERFACE OpenGL::GL OpenGL::GLU GLEW::GLEW)
  endif()
elseif(Viskores_ENABLE_OSMESA_CONTEXT)
  target_link_libraries(viskores_rendering_gl_context
                        INTERFACE OpenGL::GL OpenGL::GLU GLEW::GLEW)
elseif(Viskores_ENABLE_EGL_CONTEXT)
  target_link_libraries(viskores_rendering_gl_context
                        INTERFACE OpenGL::OpenGL OpenGL::EGL GLEW::GLEW)
endif()
