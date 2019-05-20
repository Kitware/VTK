find_path(utf8cpp_INCLUDE_DIR
  NAMES utf8.h
  PATH_SUFFIXES utf8cpp
  DOC "utf8cpp include directory")
mark_as_advanced(utf8cpp_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(utf8cpp
  REQUIRED_VARS utf8cpp_INCLUDE_DIR)

if (utf8cpp_FOUND)
  set(utf8cpp_INCLUDE_DIRS "${utf8cpp_INCLUDE_DIR}")

  if (NOT TARGET utf8cpp::utf8cpp)
    add_library(utf8cpp::utf8cpp INTERFACE IMPORTED)
    set_target_properties(utf8cpp::utf8cpp PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${utf8cpp_INCLUDE_DIR}")
  endif ()
endif ()
