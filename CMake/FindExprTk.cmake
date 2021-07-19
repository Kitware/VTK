# - Try to find ExprTk lib
#
# This module supports requiring a minimum version, e.g. you can do
#   find_package(ExprTk 2.71828)
# to require version 2.71828 or newer of ExprTk.
#
# Once done this will define
#
#  ExprTk_FOUND - system has eigen lib with correct version
#  ExprTk_INCLUDE_DIRS - the eigen include directory
#  ExprTk_VERSION - eigen version
#
# And the following imported target:
#
#  ExprTk::ExprTk

find_path(ExprTk_INCLUDE_DIR
  NAMES exprtk.hpp
  DOC "Path to ExprTk header")
mark_as_advanced(ExprTk_INCLUDE_DIR)

if (ExprTk_INCLUDE_DIR)
  file(STRINGS "${ExprTk_INCLUDE_DIR}/exprtk.hpp" _exprtk_version_header
    REGEX "static const char\\* version")
  string(REGEX MATCH "static const char\\* version = \"([0-9.]+)\"" _exprtk_version_match "${_exprtk_version_header}")
  set(ExprTk_VERSION "${CMAKE_MATCH_1}")
  unset(_exprtk_version_header)
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
