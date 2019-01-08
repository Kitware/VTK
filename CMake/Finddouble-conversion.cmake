find_path(double-conversion_INCLUDE_DIR
  NAMES
    double-conversion.h
  PATH_SUFFIXES
    double-conversion
  DOC "double-conversion include directory")
mark_as_advanced(double-conversion_INCLUDE_DIR)

find_library(double-conversion_LIBRARY
  NAMES
    double-conversion
  DOC "double-conversion library")
mark_as_advanced(double-conversion_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(double-conversion
  REQUIRED_VARS double-conversion_LIBRARY double-conversion_INCLUDE_DIR)

if (double-conversion_FOUND)
  set(double-conversion_INCLUDE_DIRS "${double-conversion_INCLUDE_DIR}")
  set(double-conversion_LIBRARIES "${double-conversion_LIBRARY}")

  if (NOT TARGET double-conversion::double-conversion)
    add_library(double-conversion::double-conversion UNKNOWN IMPORTED)
    set_target_properties(double-conversion::double-conversion PROPERTIES
      IMPORTED_LOCATION "${double-conversion_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${double-conversion_INCLUDE_DIR}")
  endif ()
endif ()
