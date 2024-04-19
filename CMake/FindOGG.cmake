find_path(OGG_INCLUDE_DIR
  NAMES
    ogg/ogg.h
  DOC "ogg include directory")
mark_as_advanced(OGG_INCLUDE_DIR)

find_library(OGG_LIBRARY
  NAMES
    ogg
  DOC "ogg library")
mark_as_advanced(OGG_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OGG REQUIRED_VARS OGG_LIBRARY OGG_INCLUDE_DIR)

if (OGG_FOUND)
  set(OGG_LIBRARIES "${OGG_LIBRARY}")
  set(OGG_INCLUDE_DIRS "${OGG_INCLUDE_DIR}")

  if (NOT TARGET OGG::OGG)
    add_library(OGG::OGG UNKNOWN IMPORTED)
    set_target_properties(OGG::OGG PROPERTIES
      IMPORTED_LOCATION "${OGG_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${OGG_INCLUDE_DIR}")
  endif ()
endif ()
