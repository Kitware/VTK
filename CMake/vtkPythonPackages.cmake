#------------------------------------------------------------------------------
# Function used to copy a Python package into the binary directory and compile
# it.
# package     :- The name of the Python package.
# source_dir  :- The directory containing the Python source.
# binary_dir  :- The directory to copy files to and compile into.
#------------------------------------------------------------------------------
function(build_python_package package source_dir binary_dir)

  set (copy-complete "${binary_dir}/${package}-copy-complete")

  copy_files_recursive("${source_dir}"
    DESTINATION "${binary_dir}"
    REGEX "^(.*\\.py)$"
    OUTPUT ${copy-complete}
    LABEL "Copying ${package} files")

  set(CMAKE_CONFIGURABLE_FILE_CONTENT
    "from compileall import compile_dir
compile_dir('${binary_dir}')
file = open('${binary_dir}/${package}_complete', 'w')
file.write('Done')
")
  configure_file(${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in
    "${CMAKE_CURRENT_BINARY_DIR}/compile_py" @ONLY)
  unset(CMAKE_CONFIGURABLE_FILE_CONTENT)

  add_custom_command(
    COMMAND ${PYTHON_EXECUTABLE} ARGS ${CMAKE_CURRENT_BINARY_DIR}/compile_py
    COMMAND ${PYTHON_EXECUTABLE} ARGS -O ${CMAKE_CURRENT_BINARY_DIR}/compile_py
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/compile_py
            ${copy-complete}
    OUTPUT  "${binary_dir}/${package}_complete"
    COMMENT "Compiling Python files")

  add_custom_target(${package} ALL DEPENDS "${binary_dir}/${package}_complete")
endfunction()

#------------------------------------------------------------------------------
# Function used to copy arbitrary files matching certain patterns.
# Usage:
# copy_files_recursive(<source-dir>
#   DESTINATION <destination-dir>
#   [LABEL "<label to use>"]
#   [OUTPUT "<file generated to mark end of copying>"]
#   [REGEX <regex> [EXCLUDE]]
#   )
# One can specify multiple REGEX or REGEX <regex> EXCLUDE arguments.
#------------------------------------------------------------------------------
function(copy_files_recursive source-dir)
  set (dest-dir)
  set (patterns)
  set (exclude-patterns)
  set (output-file)
  set (label "Copying files")

  set (doing "")
  foreach (arg ${ARGN})
    if (arg MATCHES "^(DESTINATION|REGEX|OUTPUT|LABEL)$")
      set (doing "${arg}")
    elseif ("${doing}" STREQUAL "DESTINATION")
      set (doing "")
      set (dest-dir "${arg}")
    elseif ("${doing}" STREQUAL "REGEX")
      set (doing "SET")
      list (APPEND patterns "${arg}")
    elseif (("${arg}" STREQUAL "EXCLUDE") AND ("${doing}" STREQUAL "SET"))
      set (doing "")
      list (GET patterns -1 cur-pattern)
      list (REMOVE_AT patterns -1)
      list (APPEND exclude-patterns "${cur-pattern}")
    elseif ("${doing}" STREQUAL "OUTPUT")
      set (doing "")
      set (output-file "${arg}")
    elseif ("${doing}" STREQUAL "LABEL")
      set (doing "")
      set (label "${arg}")
    else()
      message(AUTHOR_WARNING "Unknown argument [${arg}]")
    endif()
  endforeach()

  set (match-regex)
  foreach (_item ${patterns})
    if (match-regex)
      set (match-regex "${match-regex}")
    endif()
    set (match-regex "${match-regex}${_item}")
  endforeach()

  set (exclude-regex)
  foreach (_item ${exclude-patterns})
    if (exclude-regex)
      set (exclude-regex "${exclude-regex}|")
    endif()
    set (exclude-regex "${exclude-regex}${_item}")
  endforeach()

  file(GLOB_RECURSE _all_files RELATIVE "${source-dir}" "${source-dir}/*")

  set (all_files)
  set (copy-commands)
  foreach (_file ${_all_files})
    if (exclude-regex AND ("${_file}" MATCHES "${exclude-regex}"))
      # skip
    elseif ("${_file}" MATCHES "${match-regex}")
      set (in-file "${source-dir}/${_file}")
      set (out-file "${dest-dir}/${_file}")
      get_filename_component(out-path ${out-file} PATH)
      list (APPEND all_files ${in-file})
      set (copy-commands "${copy-commands}
        file(COPY \"${in-file}\" DESTINATION \"${out-path}\")")
    endif()
  endforeach()

  get_filename_component(_name ${output-file} NAME)
  set(CMAKE_CONFIGURABLE_FILE_CONTENT ${copy-commands})
  configure_file(${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in
    "${CMAKE_CURRENT_BINARY_DIR}/${_name}.cfr.cmake" @ONLY)
  unset(CMAKE_CONFIGURABLE_FILE_CONTENT)

  add_custom_command(OUTPUT ${output-file}
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/${_name}.cfr.cmake"
    COMMAND ${CMAKE_COMMAND} -E touch ${output-file}
    DEPENDS ${all_files}
            "${CMAKE_CURRENT_BINARY_DIR}/${_name}.cfr.cmake"
    COMMENT ${label})
endfunction()
