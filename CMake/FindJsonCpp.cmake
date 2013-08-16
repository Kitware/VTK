# Find the JsonCpp include files and library.
#
# JsonCpp is a C++ library that can read/write JSON (JavaScript Object Notation)
# documents. See http://jsoncpp.sourceforge.net/ for more details.
#
# This module defines:
# JsonCpp_INCLUDE_DIRS - where to find json/json.h
# JsonCpp_LIBRARIES - the libraries to link against to use JsonCpp
# JsonCpp_FOUND - if false the library was not found.

find_path(JsonCpp_INCLUDE_DIR "json/json.h"
  PATH_SUFFIXES "jsoncpp"
  DOC "Specify the JsonCpp include directory here")

find_library(JsonCpp_LIBRARY
  NAMES jsoncpp
  PATHS
  DOC "Specify the JsonCpp library here")
set(JsonCpp_INCLUDE_DIRS ${JsonCpp_INCLUDE_DIR})
set(JsonCpp_LIBRARIES "${JsonCpp_LIBRARY}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JsonCpp DEFAULT_MSG
  JsonCpp_LIBRARIES JsonCpp_INCLUDE_DIRS)

mark_as_advanced(JsonCpp_INCLUDE_DIR JsonCpp_LIBRARY)
