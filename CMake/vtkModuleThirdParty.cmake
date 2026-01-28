cmake_policy(PUSH)
if (POLICY CMP0159)
  cmake_policy(SET CMP0159 NEW)
endif ()

function (vtk_third_party_extract)
  cmake_parse_arguments(_vtk_third_party_extract
    ""
    "FILE"
    "TAGS"
    ${ARGN})

  if (_vtk_third_party_extract_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for `_vtk_third_party_extract`: "
      "${_vtk_third_party_extract_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT _vtk_third_party_extract_FILE)
    message(FATAL_ERROR
      "The `FILE` argument is required.")
  endif ()

  file(STRINGS "${_vtk_third_party_extract_FILE}"
    _vtk_third_party_extract_lines
    REGEX "readonly ")
  foreach (_vtk_third_party_extract_line IN LISTS _vtk_third_party_extract_lines)
    if (_vtk_third_party_extract_line MATCHES "^readonly ([a-z]+)=\"([^\"]+)\"$")
      set("_vtk_third_party_extract_key_${CMAKE_MATCH_1}"
        "${CMAKE_MATCH_2}")
    endif ()
  endforeach ()

  set(_vtk_third_party_extract_var "")
  set(_vtk_third_party_extract_key 1)
  foreach (_vtk_third_party_extract_arg IN LISTS _vtk_third_party_extract_TAGS)
    if (_vtk_third_party_extract_key)
      set(_vtk_third_party_extract_var "${_vtk_third_party_extract_arg}")
      set(_vtk_third_party_extract_key 0)
    else ()
      if (NOT DEFINED "_vtk_third_party_extract_key_${_vtk_third_party_extract_var}")
        message(FATAL_ERROR
          "The key `${_vtk_third_party_extract_var}` is not present in "
          "`${_vtk_third_party_extract_FILE}")
      endif ()
      set("${_vtk_third_party_extract_arg}"
        "${_vtk_third_party_extract_key_${_vtk_third_party_extract_var}}"
        PARENT_SCOPE)
      set(_vtk_third_party_extract_key 1)
    endif ()
  endforeach ()

  if (NOT _vtk_third_party_extract_key)
    message(FATAL_ERROR
      "`TAGS` must be given a set of tag/variable pairs.")
  endif ()
endfunction ()

cmake_policy(POP)
