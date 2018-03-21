# - Find GL2PS library
# Find the native GL2PS includes and library
# This module defines
#  GL2PS_INCLUDE_DIR, where to find tiff.h, etc.
#  GL2PS_LIBRARIES, libraries to link against to use GL2PS.
#  GL2PS_FOUND, If false, do not try to use GL2PS.
# also defined, but not for general use are
#  GL2PS_LIBRARY, where to find the GL2PS library.

#=============================================================================
# Copyright 2009 Kitware, Inc.
# Copyright 2009 Mathieu Malaterre <mathieu.malaterre@gmail.com>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distributed this file outside of CMake, substitute the full
#  License text for the above reference.)

find_path(GL2PS_INCLUDE_DIR gl2ps.h)

find_library(GL2PS_LIBRARY NAMES gl2ps)

if(GL2PS_INCLUDE_DIR)
  file(STRINGS "${GL2PS_INCLUDE_DIR}/gl2ps.h" _GL2PS_H_CONTENTS REGEX "#define[ \t]+GL2PS_(MAJOR|MINOR|PATCH)_VERSION[ \t]+")
  foreach(v MAJOR MINOR PATCH)
    if("${_GL2PS_H_CONTENTS}" MATCHES "#define[ \t]+GL2PS_${v}_VERSION[ \t]+([0-9]+)")
      set(GL2PS_${v}_VERSION "${CMAKE_MATCH_1}")
    else()
      set(GL2PS_${v}_VERSION 0)
    endif()
  endforeach()
  unset(_GL2PS_H_CONTENTS)
  set(GL2PS_VERSION "${GL2PS_MAJOR_VERSION}.${GL2PS_MINOR_VERSION}.${GL2PS_PATCH_VERSION}")
endif()

# handle the QUIETLY and REQUIRED arguments and set GL2PS_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GL2PS
  REQUIRED_VARS GL2PS_LIBRARY GL2PS_INCLUDE_DIR
  VERSION_VAR GL2PS_VERSION
  )

if(GL2PS_FOUND)
  set( GL2PS_LIBRARIES ${GL2PS_LIBRARY} )
endif()

mark_as_advanced(GL2PS_INCLUDE_DIR GL2PS_LIBRARY)
