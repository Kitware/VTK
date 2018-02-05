find_path(THEORA_INCLUDE_DIR
  NAMES
    theora/theora.h)

get_filename_component(computed_theora_root "${THEORA_INCLUDE_DIR}" DIRECTORY)

find_library(THEORA_enc_LIBRARY
  NAMES
    theoraenc
  HINTS
    "${computed_theora_root}/lib"
    "${computed_theora_root}/lib64")

find_library(THEORA_dec_LIBRARY
  NAMES
    theoradec
  HINTS
    "${computed_theora_root}/lib"
    "${computed_theora_root}/lib64")

set(THEORA_LIBRARIES ${THEORA_enc_LIBRARY} ${THEORA_dec_LIBRARY})

add_library(theora::enc UNKNOWN IMPORTED)
set_target_properties(theora::enc
  PROPERTIES
  IMPORTED_LOCATION ${THEORA_enc_LIBRARY}
  INTERFACE_INCLUDE_DIRECTORIES ${THEORA_INCLUDE_DIR})

add_library(theora::dec UNKNOWN IMPORTED)
set_target_properties(theora::dec
  PROPERTIES
  IMPORTED_LOCATION ${THEORA_dec_LIBRARY}
  INTERFACE_INCLUDE_DIRECTORIES ${THEORA_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(theora DEFAULT_MSG THEORA_enc_LIBRARY THEORA_dec_LIBRARY THEORA_INCLUDE_DIR)

mark_as_advanced(THEORA_enc_LIBRARY THEORA_dec_LIBRARY THEORA_INCLUDE_DIR)
