find_path(loguru_INCLUDE_DIR
  NAMES
    loguru.hpp
  DOC "loguru include directory")
mark_as_advanced(loguru_INCLUDE_DIR)

find_library(loguru_LIBRARY
  NAMES
    loguru
  DOC "loguru library")
mark_as_advanced(loguru_LIBRARY)

# TODO: extract version from `loguru.hpp`

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(loguru
  REQUIRED_VARS loguru_LIBRARY loguru_INCLUDE_DIR)

if (loguru_FOUND)
  set(loguru_INCLUDE_DIRS "${loguru_INCLUDE_DIR}")
  set(loguru_LIBRARIES "${loguru_LIBRARY}")
  if (NOT TARGETS loguru::loguru)
    add_library(loguru::loguru UNKNOWN IMPORTED)
    set_target_properties(loguru::loguru PROPERTIES
      IMPORTED_LOCATION "${loguru_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${loguru_INCLUDE_DIR}")
  endif ()
endif ()
