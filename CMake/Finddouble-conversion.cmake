find_path(DOUBLE-CONVERSION_INCLUDE_DIR
  NAMES
    double-conversion.h
  PATH_SUFFIXES
    double-conversion)

find_library(DOUBLE-CONVERSION_LIBRARY
  NAMES
    double-conversion
    )

set(DOUBLE-CONVERSION_LIBRARIES "${DOUBLE-CONVERSION_LIBRARY}")

add_library(double-conversion::double-conversion UNKNOWN IMPORTED)
set_target_properties(double-conversion::double-conversion
  PROPERTIES
  IMPORTED_LOCATION ${DOUBLE-CONVERSION_LIBRARY}
  INTERFACE_INCLUDE_DIRECTORIES ${DOUBLE-CONVERSION_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(double-conversion
  DOUBLE-CONVERSION_LIBRARIES
  DOUBLE-CONVERSION_INCLUDE_DIR
)

mark_as_advanced(DOUBLE-CONVERSION_LIBRARY DOUBLE-CONVERSION_INCLUDE_DIR)
