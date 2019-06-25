find_path(JsonCpp_INCLUDE_DIR "json/json.h"
  PATH_SUFFIXES "jsoncpp"
  DOC "jsoncpp include directory")
mark_as_advanced(JsonCpp_INCLUDE_DIR)

find_library(JsonCpp_LIBRARY
  NAMES jsoncpp
  DOC "jsoncpp library")
mark_as_advanced(JsonCpp_LIBRARY)

if (JsonCpp_INCLUDE_DIR AND EXISTS "${JsonCpp_INCLUDE_DIR}/json/version.h")
  file(STRINGS "${JsonCpp_INCLUDE_DIR}/json/version.h" _JsonCpp_version_lines
    REGEX "JSONCPP_VERSION_[A-Z]+")
  string(REGEX REPLACE ".*# *define +JSONCPP_VERSION_MAJOR +([0-9]+).*" "\\1" _JsonCpp_version_major "${_JsonCpp_version_lines}")
  string(REGEX REPLACE ".*# *define +JSONCPP_VERSION_MINOR +([0-9]+).*" "\\1" _JsonCpp_version_minor "${_JsonCpp_version_lines}")
  string(REGEX REPLACE ".*# *define +JSONCPP_VERSION_PATCH +([0-9]+).*" "\\1" _JsonCpp_version_patch "${_JsonCpp_version_lines}")
  set(JsonCpp_VERSION "${_JsonCpp_version_major}.${_JsonCpp_version_minor}.${_JsonCpp_version_patch}")
  unset(_JsonCpp_version_major)
  unset(_JsonCpp_version_minor)
  unset(_JsonCpp_version_patch)
  unset(_JsonCpp_version_lines)
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JsonCpp
  REQUIRED_VARS JsonCpp_LIBRARY JsonCpp_INCLUDE_DIR
  VERSION_VAR JsonCpp_VERSION)

if (JsonCpp_FOUND)
  set(JsonCpp_INCLUDE_DIRS "${JsonCpp_INCLUDE_DIR}")
  set(JsonCpp_LIBRARIES "${JsonCpp_LIBRARY}")

  if (NOT TARGET JsonCpp::JsonCpp)
    include(vtkDetectLibraryType)
    vtk_detect_library_type(jsoncpp_library_type
      PATH "${JsonCpp_LIBRARY}")
    add_library(JsonCpp::JsonCpp "${jsoncpp_library_type}" IMPORTED)
    unset(jsoncpp_library_type)
    set_target_properties(JsonCpp::JsonCpp PROPERTIES
      IMPORTED_LOCATION "${JsonCpp_LIBRARY}"
      IMPORTED_IMPLIB "${JsonCpp_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${JsonCpp_INCLUDE_DIR}")
  endif ()
endif ()
