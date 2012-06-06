# - Find FontConfig library
# Find the FontConfig includes and library
# This module defines
#  FONTCONFIG_INCLUDE_DIR, where to find fontconfig.h
#  FONTCONFIG_LIBRARIES, libraries to link against to use the FontConfig API.
#  FONTCONFIG_FOUND, If false, do not try to use FontConfig.

#=============================================================================
# Copyright 2012 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of VTK, substitute the full
#  License text for the above reference.)

find_path(FONTCONFIG_INCLUDE_DIR fontconfig/fontconfig.h)

find_library(FONTCONFIG_LIBRARY NAMES fontconfig)

# handle the QUIETLY and REQUIRED arguments and set FONTCONFIG_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FontConfig DEFAULT_MSG
  FONTCONFIG_LIBRARY  FONTCONFIG_INCLUDE_DIR)

if(FONTCONFIG_FOUND)
  set( FONTCONFIG_LIBRARIES ${FONTCONFIG_LIBRARY} )
endif()

mark_as_advanced(FONTCONFIG_INCLUDE_DIR FONTCONFIG_LIBRARY FONTCONFIG_LIBRARIES)
