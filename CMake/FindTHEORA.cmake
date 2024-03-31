find_path(THEORA_INCLUDE_DIR
  NAMES
    theora/theora.h
  DOC "theora include directory")
mark_as_advanced(THEORA_INCLUDE_DIR)

get_filename_component(computed_theora_root "${THEORA_INCLUDE_DIR}" DIRECTORY)

set(THEORA_required_libs THEORA_LIBRARY)

find_library(THEORA_LIBRARY
  NAMES
    theora
  HINTS
    "${computed_theora_root}/lib"
    "${computed_theora_root}/lib64"
  DOC "theora library")
mark_as_advanced(THEORA_LIBRARY)

# When built from source by MSVC theora
# does not come with the encoder or decoder
# libraries but vendors the same symbols
# that VTK consumes in a single lib named theora
# so skip searching for them on Windows
if (NOT WIN32 OR (WIN32 AND (MSYS OR MINGW)))
  list(APPEND THEORA_required_libs
                           THEORA_dec_LIBRARY
                           THEORA_enc_LIBRARY)

  find_library(THEORA_enc_LIBRARY
    NAMES
      theoraenc
    HINTS
      "${computed_theora_root}/lib"
      "${computed_theora_root}/lib64"
    DOC "theora encoding library")
  mark_as_advanced(THEORA_enc_LIBRARY)

  find_library(THEORA_dec_LIBRARY
    NAMES
      theoradec
    HINTS
      "${computed_theora_root}/lib"
      "${computed_theora_root}/lib64"
    DOC "theora decoding library")
  mark_as_advanced(THEORA_dec_LIBRARY)
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(THEORA
  REQUIRED_VARS ${THEORA_required_libs} THEORA_INCLUDE_DIR)

if (THEORA_FOUND)
  set(THEORA_LIBRARIES "${THEORA_LIBRARY}" "${THEORA_enc_LIBRARY}" "${THEORA_dec_LIBRARY}")
  set(THEORA_INCLUDE_DIRS "${THEORA_INCLUDE_DIR}")

  if (NOT TARGET THEORA::THEORA)
    add_library(THEORA::THEORA UNKNOWN IMPORTED)
    set_target_properties(THEORA::THEORA PROPERTIES
      IMPORTED_LOCATION "${THEORA_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES ${THEORA_INCLUDE_DIR}
      INTERFACE_LINK_LIBRARIES VTK::ogg)
  endif ()

  if (NOT TARGET THEORA::ENC)
    add_library(THEORA::ENC UNKNOWN IMPORTED)
    set_target_properties(THEORA::ENC PROPERTIES
      IMPORTED_LOCATION "${THEORA_enc_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES ${THEORA_INCLUDE_DIR}
      INTERFACE_LINK_LIBRARIES VTK::ogg)
  endif()

  if (NOT TARGET THEORA::DEC)
    add_library(THEORA::DEC UNKNOWN IMPORTED)
    set_target_properties(THEORA::DEC PROPERTIES
      IMPORTED_LOCATION "${THEORA_dec_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES ${THEORA_INCLUDE_DIR})
  endif()
endif ()
