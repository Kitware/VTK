find_path(SQLite3_INCLUDE_DIR NAMES sqlite3.h)

get_filename_component(POSSIBLE_SQLite_ROOT "${SQLite3_INCLUDE_DIR}" DIRECTORY)

find_library(SQLite3_LIBRARY
  NAMES sqlite3
  HINTS ${POSSIBLE_SQLite_ROOT}/lib
        ${POSSIBLE_SQLite_ROOT}/lib64)


set(SQLite3_LIBRARIES ${SQLite3_LIBRARY})

add_library(sqlite3::sqlite3 UNKNOWN IMPORTED)
set_target_properties(sqlite3::sqlite3
  PROPERTIES
    IMPORTED_LOCATION ${SQLite3_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES ${SQLite3_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SQLite3 DEFAULT_MSG SQLite3_LIBRARY SQLite3_INCLUDE_DIR)

mark_as_advanced(SQLite3_INCLUDE_DIR SQLite3_LIBRARY)
