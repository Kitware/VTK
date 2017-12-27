#[==[.md
# vtkHashSource

This module contains the `vtk_hash_source` function which may be used to
generate a hash from a file and place that in a generated header.
#]==]

set(_vtkHashSource_script_file "${CMAKE_CURRENT_LIST_FILE}")

include(CMakeParseArguments)

#[==[.md
# `vtk_hash_source`

Add a rule to turn a file into a MD5 hash and place that in a C string.

```
vtk_hash_source(
  INPUT          <input>
  [NAME          <name>]
  [ALGORITHM     <algorithm>]
  [HEADER_OUTPUT <header>])
```

The only required variable is `INPUT`.

  * `INPUT`: (Required) The path to the file to process. If a relative path
    is given, it will be interpreted as being relative to
    `CMAKE_CURRENT_SOURCE_DIR`.
  * `NAME`: This is the base name of the header file that will be generated as
    well as the variable name for the C string. It defaults to basename of the
    input suffixed with `Hash`.
  * `ALGORITHM`: This is the hashing algorithm to use. Supported values are
    MD5, SHA1, SHA224, SHA256, SHA384, and SHA512. If not specified, MD5 is assumed.
  * `HEADER_OUTPUT`: the variable to store the generated header path.
#]==]
function (vtk_hash_source)
  cmake_parse_arguments(_vtk_hash_source
    ""
    "INPUT;NAME;ALGORITHM;HEADER_OUTPUT"
    ""
    ${ARGN})

  if (_vtk_hash_source_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unrecognized arguments to vtk_hash_source: "
      "${_vtk_hash_source_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _vtk_hash_source_INPUT)
    message(FATAL_ERROR
      "Missing `INPUT` for vtk_hash_source.")
  endif ()

  if (NOT DEFINED _vtk_hash_source_NAME)
    get_filename_component(_vtk_hash_source_NAME
      "${_vtk_hash_source_INPUT}" NAME_WE)
    set(_vtk_hash_source_NAME "${_vtk_hash_source_NAME}Hash")
  endif ()

  if (NOT DEFINED _vtk_hash_source_ALGORITHM)
    set(_vtk_hash_source_ALGORITHM MD5)
  endif ()

  if (IS_ABSOLUTE "${_vtk_hash_source_INPUT}")
    set(_vtk_hash_source_input
      "${_vtk_hash_source_INPUT}")
  else ()
    set(_vtk_hash_source_input
      "${CMAKE_CURRENT_SOURCE_DIR}/${_vtk_hash_source_INPUT}")
  endif ()

  set(_vtk_hash_source_header
    "${CMAKE_CURRENT_BINARY_DIR}/${_vtk_hash_source_NAME}.h")

  add_custom_command(
    OUTPUT  "${_vtk_hash_source_header}"
    DEPENDS "${_vtkHashSource_script_file}"
            "${_vtk_hash_source_input}"
    COMMAND "${CMAKE_COMMAND}"
            "-Dinput_file=${_vtk_hash_source_input}"
            "-Doutput_file=${_vtk_hash_source_header}"
            "-Doutput_name=${_vtk_hash_source_NAME}"
            "-Dalgorithm=${_vtk_hash_source_ALGORITHM}"
            "-D_vtk_hash_source_run=ON"
            -P "${_vtkHashSource_script_file}")

  if (DEFINED _vtk_hash_source_HEADER_OUTPUT)
    set("${_vtk_hash_source_HEADER_OUTPUT}"
      "${_vtk_hash_source_header}"
      PARENT_SCOPE)
  endif ()
endfunction()

if (_vtk_hash_source_run)
  file(${algorithm} "${input_file}" file_hash)
  file(WRITE "${output_file}"
    "#ifndef ${output_name}\n #define ${output_name} \"${file_hash}\"\n#endif\n")
endif ()
