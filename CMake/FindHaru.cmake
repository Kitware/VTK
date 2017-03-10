# Find Haru PDF library
#
# This module defines
# - HARU_INCLUDE_DIR, where to find hpdf.h
# - HARU_LIBRARIES, libraries to link against to use the Haru API.
# - HARU_FOUND, If false, do not try to use Haru.
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

find_path(HARU_INCLUDE_DIR hpdf.h)

find_library(HARU_LIBRARY NAMES hpdf)

# handle the QUIETLY and REQUIRED arguments and set FONTCONFIG_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Haru DEFAULT_MSG
  HARU_LIBRARY
  HARU_INCLUDE_DIR
)

if(HARU_FOUND)
  set(HARU_LIBRARIES "${HARU_LIBRARY}")
endif()

mark_as_advanced(HARU_INCLUDE_DIR HARU_LIBRARY HARU_LIBRARIES)
