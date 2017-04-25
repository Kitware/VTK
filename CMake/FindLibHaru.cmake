# Find LibHaru PDF library
#
# This module defines
# - LIBHARU_INCLUDE_DIR, where to find hpdf.h
# - LIBHARU_LIBRARIES, libraries to link against to use the LibHaru API.
# - LIBHARU_FOUND, If false, do not try to use LibHaru.
#=============================================================================
# Copyright 2017 Kitware, Inc.
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

find_path(LIBHARU_INCLUDE_DIR hpdf.h)

find_library(LIBHARU_LIBRARY NAMES hpdf)

# handle the QUIETLY and REQUIRED arguments and set FONTCONFIG_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibHaru DEFAULT_MSG
  LIBHARU_LIBRARY
  LIBHARU_INCLUDE_DIR
)

if(LIBHARU_FOUND)
  set(LIBHARU_LIBRARIES "${LIBHARU_LIBRARY}")
endif()

mark_as_advanced(LIBHARU_INCLUDE_DIR LIBHARU_LIBRARY LIBHARU_LIBRARIES)
