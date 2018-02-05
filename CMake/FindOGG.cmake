find_path(OGG_INCLUDE_DIR
  NAMES
    ogg/ogg.h)

get_filename_component(computed_ogg_root "${OGG_INCLUDE_DIR}" DIRECTORY)

find_library(OGG_LIBRARY
  NAMES
    ogg
  HINTS
    ${computed_ogg_root}/lib
    ${computed_ogg_root}/lib64
    )

set(OGG_LIBRARIES ${OGG_LIBRARY})

add_library(ogg::ogg UNKNOWN IMPORTED)
set_target_properties(ogg::ogg
  PROPERTIES
  IMPORTED_LOCATION ${OGG_LIBRARY}
  INTERFACE_INCLUDE_DIRECTORIES ${OGG_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ogg DEFAULT_MSG OGG_LIBRARY OGG_INCLUDE_DIR)

mark_as_advanced(OGG_LIBRARY OGG_INCLUDE_DIR)
