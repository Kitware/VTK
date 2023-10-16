# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-FileCopyrightText: Copyright 2012 Kitware, Inc.
# SPDX-License-Identifier: BSD-3-Clause
# - Find FontConfig library
# Find the FontConfig includes and library
# This module defines
#  FONTCONFIG_INCLUDE_DIR, where to find fontconfig.h
#  FONTCONFIG_LIBRARIES, libraries to link against to use the FontConfig API.
#  FONTCONFIG_FOUND, If false, do not try to use FontConfig.
find_path(FONTCONFIG_INCLUDE_DIR fontconfig/fontconfig.h)

find_library(FONTCONFIG_LIBRARY NAMES fontconfig)

# handle the QUIETLY and REQUIRED arguments and set FONTCONFIG_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FontConfig DEFAULT_MSG
  FONTCONFIG_LIBRARY  FONTCONFIG_INCLUDE_DIR)

if(FONTCONFIG_FOUND)
  if (NOT TARGET FontConfig::FontConfig)
    add_library(FontConfig::FontConfig UNKNOWN IMPORTED)
    set_target_properties(FontConfig::FontConfig PROPERTIES
      IMPORTED_LOCATION "${FONTCONFIG_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${FONTCONFIG_INCLUDE_DIR}")
  endif ()
  set( FONTCONFIG_LIBRARIES ${FONTCONFIG_LIBRARY} )
endif()

mark_as_advanced(FONTCONFIG_INCLUDE_DIR FONTCONFIG_LIBRARY FONTCONFIG_LIBRARIES)
