#[==[
Provides the following variables:

  * `ODBC_INCLUDE_DIRS`: Include directories necessary to use ODBC.
  * `ODBC_LIBRARIES`: Libraries necessary to use ODBC.
  * A `ODBC::ODBC` imported target.
#]==]

# No .pc files are shipped with ODBC on Windows.
set(_ODBC_use_pkgconfig 0)
if (NOT MSVC)
  find_package(PkgConfig QUIET)
  if (PkgConfig_FOUND)
    set(_ODBC_use_pkgconfig 1)
  endif ()
endif ()

if (_ODBC_use_pkgconfig)
  pkg_check_modules(_iodbc "libiodbc" QUIET IMPORTED_TARGET)
  unset(_odbc_target)
  if (NOT _iodbc_FOUND)
    pkg_check_modules(_unixodbc "odbc" QUIET IMPORTED_TARGET)
    if (_unixodbc_FOUND)
      set(_odbc_target "_unixodbc")
    endif ()
  else ()
    set(_odbc_target "_iodbc")
  endif ()

  set(ODBC_FOUND 0)
  if (_odbc_target)
    set(ODBC_FOUND 1)
    set(ODBC_INCLUDE_DIRS ${${_odbc_target}_INCLUDE_DIRS})
    set(ODBC_LIBRARIES ${${_odbc_target}_LINK_LIBRARIES})
    if (NOT TARGET ODBC::ODBC)
      add_library(ODBC::ODBC INTERFACE IMPORTED)
      target_link_libraries(ODBC::ODBC
        INTERFACE "PkgConfig::${_odbc_target}")
      if (MINGW AND _odbc_target STREQUAL "_unixodbc")
        set_target_properties(ODBC::ODBC PROPERTIES
          INTERFACE_COMPILE_DEFINITIONS SQL_WCHART_CONVERT)
      endif ()
    endif ()
  endif ()
  unset(_odbc_target)
else ()
  find_path(ODBC_INCLUDE_DIR
    NAMES sql.h
    PATHS
      "C:/Program Files/ODBC"
      "C:/ODBC"
    PATH_SUFFIXES include include/odbc libiodbc
    DOC "Location of sql.h")
  mark_as_advanced(ODBC_INCLUDE_DIR)
  find_library(ODBC_LIBRARY
    NAMES odbc iodbc unixodbc odbc32
    PATHS
      "C:/Program Files/ODBC"
      "C:/ODBC"
    PATH_SUFFIXES lib lib/debug
    DOC "Location of the ODBC library")
  mark_as_advanced(ODBC_LIBRARY)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(ODBC
    REQUIRED_VARS ODBC_INCLUDE_DIR ODBC_LIBRARY)

  if (ODBC_FOUND)
    set(ODBC_INCLUDE_DIRS "${ODBC_INCLUDE_DIR}")
    set(ODBC_LIBRARIES "${ODBC_LIBRARY}")
    if (NOT TARGET ODBC::ODBC)
      add_library(ODBC::ODBC UNKNOWN IMPORTED)
      set_target_properties(ODBC::ODBC PROPERTIES
        IMPORTED_LOCATION "${ODBC_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${ODBC_INCLUDE_DIR}")
    endif ()
  endif ()
endif ()
unset(_ODBC_use_pkgconfig)
