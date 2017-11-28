#------------------------------------------------------------------------------
# Function used to copy arbitrary files matching certain patterns.
# Usage:
# copy_files_recursive(<source-dir>
#   DESTINATION <destination-dir>
#   [LABEL "<label to use>"]
#   [OUTPUT "<file generated to mark end of copying>"]
#   [REGEX <regex> [EXCLUDE]]
#   [DEPENDS [depends ...]]
#   )
#
# One can specify multiple REGEX or REGEX <regex> EXCLUDE arguments.
#------------------------------------------------------------------------------
function(copy_files_recursive source-dir)
  set (dest-dir)
  set (patterns)
  set (exclude-patterns)
  set (output-file)
  set (extra_depends)
  set (label "Copying files")

  set (doing "")
  foreach (arg IN LISTS ARGN)
    if (arg MATCHES "^(DESTINATION|REGEX|OUTPUT|LABEL|DEPENDS)$")
      set (doing "${arg}")
    elseif ("x${doing}" STREQUAL "xDESTINATION")
      set (doing "")
      set (dest-dir "${arg}")
    elseif ("x${doing}" STREQUAL "xREGEX")
      set (doing "SET")
      list (APPEND patterns "${arg}")
    elseif (("x${arg}" STREQUAL "xEXCLUDE") AND ("x${doing}" STREQUAL "xSET"))
      set (doing "")
      list (GET patterns -1 cur-pattern)
      list (REMOVE_AT patterns -1)
      list (APPEND exclude-patterns "${cur-pattern}")
    elseif ("x${doing}" STREQUAL "xOUTPUT")
      set (doing "")
      set (output-file "${arg}")
    elseif ("x${doing}" STREQUAL "xLABEL")
      set (doing "")
      set (label "${arg}")
    elseif ("x${doing}" STREQUAL "xDEPENDS")
      list(APPEND extra_depends "${arg}")
    else()
      message(AUTHOR_WARNING "Unknown argument [${arg}]")
    endif()
  endforeach()

  file(GLOB_RECURSE _all_files RELATIVE "${source-dir}" "${source-dir}/*")
  set(extra_args)
  foreach(_item IN LISTS patterns)
    # need to escape "\" since we're writing these out to a cmake file.
    string(REPLACE "\\" "\\\\" _item "${_item}")
    set(extra_args "${extra_args} REGEX \"${_item}\"")
  endforeach()
  foreach(_item IN LISTS exclude-patterns)
    string(REPLACE "\\" "\\\\" _item "${_item}")
    set(extra_args "${extra_args} REGEX \"${_item}\" EXCLUDE")
  endforeach()
  if(extra_args)
    set(extra_args "FILES_MATCHING ${extra_args}")
  endif()
  set (copy-commands "file(COPY \${SRCDIR} DESTINATION \${OUTDIR} ${extra_args})")

  # Let's now build a list of files matching the selection criteria
  # so we can add dependencies on those.
  set(all_files)
  string(REPLACE ";" "|" match-regex "${patterns}")
  string(REPLACE ";" "|" exclude-regex "${exclude-patterns}")
  foreach (_file ${_all_files})
    if (exclude-regex AND ("${_file}" MATCHES "${exclude-regex}"))
      # skip
    elseif ("${_file}" MATCHES "${match-regex}")
      set (in-file "${source-dir}/${_file}")
      list (APPEND all_files ${in-file})
    endif()
  endforeach()

  get_filename_component(_name ${output-file} NAME)
  set(CMAKE_CONFIGURABLE_FILE_CONTENT ${copy-commands})
  configure_file(${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in
    "${CMAKE_CURRENT_BINARY_DIR}/${_name}.cfr.cmake" @ONLY)
  unset(CMAKE_CONFIGURABLE_FILE_CONTENT)

  add_custom_command(OUTPUT ${output-file}
    COMMAND ${CMAKE_COMMAND} -DOUTDIR=${dest-dir}
                             -DSRCDIR=${source-dir}/
                             -P ${CMAKE_CURRENT_BINARY_DIR}/${_name}.cfr.cmake
    COMMAND ${CMAKE_COMMAND} -E touch ${output-file}
    DEPENDS ${all_files}
            "${CMAKE_CURRENT_BINARY_DIR}/${_name}.cfr.cmake"
            ${extra_depends}
    COMMENT ${label}
    VERBATIM)
endfunction()
