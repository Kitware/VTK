find_path(LibPROJ_INCLUDE_DIR
  NAMES proj_api.h proj.h
  DOC "libproj include directories")
mark_as_advanced(LibPROJ_INCLUDE_DIR)

find_library(LibPROJ_LIBRARY_RELEASE
  NAMES proj
  DOC "libproj release library")
mark_as_advanced(LibPROJ_LIBRARY_RELEASE)

find_library(LibPROJ_LIBRARY_DEBUG
  NAMES projd
  DOC "libproj debug library")
mark_as_advanced(LibPROJ_LIBRARY_DEBUG)

include(SelectLibraryConfigurations)
select_library_configurations(LibPROJ)

if (LibPROJ_INCLUDE_DIR)
  if (EXISTS "${LibPROJ_INCLUDE_DIR}/proj.h")
    file(STRINGS "${LibPROJ_INCLUDE_DIR}/proj.h" _libproj_version_lines REGEX "#define[ \t]+PROJ_VERSION_(MAJOR|MINOR|PATCH)")
    string(REGEX REPLACE ".*PROJ_VERSION_MAJOR *\([0-9]*\).*" "\\1" _libproj_version_major "${_libproj_version_lines}")
    string(REGEX REPLACE ".*PROJ_VERSION_MINOR *\([0-9]*\).*" "\\1" _libproj_version_minor "${_libproj_version_lines}")
    string(REGEX REPLACE ".*PROJ_VERSION_PATCH *\([0-9]*\).*" "\\1" _libproj_version_patch "${_libproj_version_lines}")
  else ()
    file(STRINGS "${LibPROJ_INCLUDE_DIR}/proj_api.h" _libproj_version_lines REGEX "#define[ \t]+PJ_VERSION")
    string(REGEX REPLACE ".*PJ_VERSION *\([0-9]*\).*" "\\1" _libproj_version "${_libproj_version_lines}")
    math(EXPR _libproj_version_major "${_libproj_version} / 100")
    math(EXPR _libproj_version_minor "(${_libproj_version} % 100) / 10")
    math(EXPR _libproj_version_patch "${_libproj_version} % 10")
  endif ()
  set(LibPROJ_VERSION "${_libproj_version_major}.${_libproj_version_minor}.${_libproj_version_patch}")
  set(LibPROJ_MAJOR_VERSION "${_libproj_version_major}")
  unset(_libproj_version_major)
  unset(_libproj_version_minor)
  unset(_libproj_version_patch)
  unset(_libproj_version)
  unset(_libproj_version_lines)
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibPROJ
  REQUIRED_VARS LibPROJ_LIBRARY LibPROJ_INCLUDE_DIR
  VERSION_VAR LibPROJ_VERSION)

if (LibPROJ_FOUND)
  set(LibPROJ_INCLUDE_DIRS "${LibPROJ_INCLUDE_DIR}")
  set(LibPROJ_LIBRARIES "${LibPROJ_LIBRARY}")

  if (NOT TARGET LibPROJ::LibPROJ)
    add_library(LibPROJ::LibPROJ UNKNOWN IMPORTED)
    set_target_properties(LibPROJ::LibPROJ PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${LibPROJ_INCLUDE_DIR}")
    if (LibPROJ_LIBRARY_RELEASE)
      set_property(TARGET LibPROJ::LibPROJ APPEND PROPERTY
        IMPORTED_CONFIGURATIONS RELEASE)
      set_target_properties(LibPROJ::LibPROJ PROPERTIES
        IMPORTED_LOCATION_RELEASE "${LibPROJ_LIBRARY_RELEASE}")
    endif ()
    if (LibPROJ_LIBRARY_DEBUG)
      set_property(TARGET LibPROJ::LibPROJ APPEND PROPERTY
        IMPORTED_CONFIGURATIONS DEBUG)
      set_target_properties(LibPROJ::LibPROJ PROPERTIES
        IMPORTED_LOCATION_DEBUG "${LibPROJ_LIBRARY_DEBUG}")
    endif ()
  endif ()
endif ()
