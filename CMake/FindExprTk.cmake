# - Try to find ExprTk lib
#
# This module supports requiring a minimum version, e.g. you can do
#   find_package(ExprTk 2.7)
# to require version 2.7 or newer of ExprTk.
#
# Once done this will define
#
#  ExprTk_FOUND - system has exprtk with correct version
#  ExprTk_INCLUDE_DIRS - the exprtk include directory
#  ExprTk_VERSION - exprtk version
#
# And the following imported target:
#
#  ExprTk::ExprTk

find_path(ExprTk_INCLUDE_DIR
  NAMES exprtk.hpp
  DOC "Path to ExprTk header")
mark_as_advanced(ExprTk_INCLUDE_DIR)

if (ExprTk_INCLUDE_DIR)
  file(STRINGS "${ExprTk_INCLUDE_DIR}/exprtk.hpp" _exprtk_version_header REGEX "\"[0-9.]+\"")
  set(ExprTk_VERSION)
  foreach (_exprtk_version_line IN LISTS _exprtk_version_header)
    if ("${ExprTk_VERSION}" STREQUAL "")
      string(REGEX MATCH [[version = "(2\.7[0-9.]+)".*$]] _exprtk_version_match "${_exprtk_version_line}")
      set(ExprTk_VERSION "${CMAKE_MATCH_1}")
    else ()
      string(REGEX MATCH "\"([0-9.]+)\".*$" _exprtk_version_match "${_exprtk_version_line}")
      set(ExprTk_VERSION "${ExprTk_VERSION}${CMAKE_MATCH_1}")
    endif ()
    if (_exprtk_version_match MATCHES "\;")
      break()
    endif ()
  endforeach ()
  if (NOT ExprTk_VERSION)
    # fallback: version in exprtk.hpp has always started with 2.7
    set(ExprTk_VERSION "2.7")
  endif ()
  unset(_exprtk_version_header)
  unset(_exprtk_version_line)
  unset(_exprtk_version_match)
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ExprTk
  REQUIRED_VARS ExprTk_INCLUDE_DIR
  VERSION_VAR ExprTk_VERSION)

if (ExprTk_FOUND)
  set(ExprTk_INCLUDE_DIRS "${ExprTk_INCLUDE_DIR}")
  if (NOT TARGET ExprTk::ExprTk)
    add_library(ExprTk::ExprTk INTERFACE IMPORTED)
    set_target_properties(ExprTk::ExprTk PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${ExprTk_INCLUDE_DIR}")
  endif ()
endif ()
