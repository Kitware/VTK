# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause

#[=======================================================================[.rst:

FindXKBCOMMON
-------------
Find the xkbcommon library.

Usage
^^^^^

Use this module by invoking

    find_package(XKBCOMMON [REQUIRED] [QUIET])

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables:

 XKBCOMMON_FOUND - system has xkbcommon

 XKBCOMMON_INCLUDE_DIRS - the xkbcommon include directories

 XKBCOMMON_LIBRARIES - xkbcommon libraries to be linked

The module also defines the following imported targets:

 XKBCOMMON::xkbcommon - imported target for the xkbcommon library

#]=======================================================================]

find_package(PkgConfig)
pkg_check_modules(PKG_XKBCOMMON QUIET IMPORTED_TARGET xkbcommon)

find_path(
  XKBCOMMON_INCLUDE_DIR
  NAMES xkbcommon/xkbcommon.h
  HINTS ${PKG_XKBCOMMON_INCLUDE_DIRS}
)
find_library(
  XKBCOMMON_LIBRARY
  NAMES xkbcommon
  HINTS ${PKG_XKBCOMMON_LIBRARY_DIRS}
)

if(NOT TARGET XKBCOMMON::xkbcommon)
  add_library(XKBCOMMON::xkbcommon UNKNOWN IMPORTED)
  set_target_properties(
    XKBCOMMON::xkbcommon
    PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${XKBCOMMON_INCLUDE_DIR}"
      IMPORTED_LOCATION "${XKBCOMMON_LIBRARY}"
  )
endif()

set(XKBCOMMON_INCLUDE_DIRS ${XKBCOMMON_INCLUDE_DIR})
set(XKBCOMMON_LIBRARIES ${XKBCOMMON_LIBRARY})

mark_as_advanced(
  XKBCOMMON_INCLUDE_DIR
  XKBCOMMON_LIBRARY
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  XKBCOMMON
  FOUND_VAR XKBCOMMON_FOUND
  REQUIRED_VARS
    XKBCOMMON_LIBRARY
    XKBCOMMON_INCLUDE_DIR
)
