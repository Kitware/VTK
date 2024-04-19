# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-FileCopyrightText: Copyright (c) 2006, 2007 Montel Laurent, <montel@kde.org>
# SPDX-FileCopyrightText: Copyright (c) 2008, 2009 Gael Guennebaud, <g.gael@free.fr>
# SPDX-FileCopyrightText: Copyright (c) 2009 Benoit Jacob <jacob.benoit.1@gmail.com>
# SPDX-License-Identifier: BSD-2-Clause

# - Try to find PEGTL lib
#
# This module supports requiring a minimum version, e.g. you can do
#   find_package(PEGTL 3.1.2)
# to require version 3.1.2 or newer of PEGTL.
#
# Once done this will define
#
#  PEGTL_FOUND - system has eigen lib with correct version
#  PEGTL_INCLUDE_DIRS - the eigen include directory
#  PEGTL_VERSION - eigen version
#
# And the following imported target:
#
#  PEGTL::PEGTL

find_path(PEGTL_INCLUDE_DIR
  NAMES pegtl/version.hpp
  PATH_SUFFIXES tao
  DOC "Path to PEGTL headers")
mark_as_advanced(PEGTL_INCLUDE_DIR)

if (PEGTL_INCLUDE_DIR)
  file(STRINGS "${PEGTL_INCLUDE_DIR}/pegtl/version.hpp" _pegtl_version_header
    REGEX "TAO_PEGTL_VERSION")
  string(REGEX MATCH "define[ \t]+TAO_PEGTL_VERSION[ \t]+\"([0-9.]+)\"" _pegtl_version_match "${_pegtl_version_header}")
  set(PEGTL_VERSION "${CMAKE_MATCH_1}")
  unset(_pegtl_version_header)
  unset(_pegtl_version_match)
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PEGTL
  REQUIRED_VARS PEGTL_INCLUDE_DIR
  VERSION_VAR PEGTL_VERSION)

if (PEGTL_FOUND)
  set(PEGTL_INCLUDE_DIRS "${PEGTL_INCLUDE_DIR}")
  if (NOT TARGET PEGTL::PEGTL)
    add_library(PEGTL::PEGTL INTERFACE IMPORTED)
    set_target_properties(PEGTL::PEGTL PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${PEGTL_INCLUDE_DIR}")
  endif ()
endif ()
