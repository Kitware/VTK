#[==[.md
# vtkEncodeString

This module contains the `vtk_encode_string` function which may be used to turn
a file into a C string. This is primarily used within a program so that the
content does not need to be retrieved from the filesystem at runtime, but can
still be developed as a standalone file.
#]==]

set(_vtkEncodeString_script_file "${CMAKE_CURRENT_LIST_FILE}")

include(CMakeParseArguments)

#[==[.md
# `vtk_encode_string`

Adds a rule to turn a file into a C string. Note that any Unicode characters
will not be replaced with escaping, so it is recommended to avoid their usage
in the input.

```
vtk_encode_string
  INPUT           <input>
  [NAME           <name>]
  [EXPORT_SYMBOL  <symbol>]
  [EXPORT_HEADER  <header>
  [HEADER_OUTPUT  <variable>]
  [SOURCE_OUTPUT  <variable>])
```

The only required variable is `INPUT`, however, it is likely that at least one
of `HEADER_OUTPUT` or `SOURCE_OUTPUT` will be required to add them to a
library.

  * `INPUT`: (Required) The path to the file to be embedded. If a relative path
    is given, it will be interpreted as being relative to
    `CMAKE_CURRENT_SOURCE_DIR`.
  * `NAME`: This is the base name of the files that will be generated as well
    as the variable name for the C string. It defaults to the basename of the
    input without extensions.
  * `EXPORT_SYMBOL`: The symbol to use for exporting the variable. By default,
    it will not be exported. If set, `EXPORT_HEADER` must also be set.
  * `EXPORT_HEADER`: The header to include for providing the given export
    symbol. If set, `EXPORT_SYMBOL` should also be set.
  * `HEADER_OUTPUT`: The variable to store the generated header path.
  * `SOURCE_OUTPUT`: The variable to store the generated source path.
#]==]
function (vtk_encode_string)
  cmake_parse_arguments(_vtk_encode_string
    ""
    "INPUT;NAME;EXPORT_SYMBOL;EXPORT_HEADER;HEADER_OUTPUT;SOURCE_OUTPUT"
    ""
    ${ARGN})

  if (_vtk_encode_string_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unrecognized arguments to vtk_encode_string: "
      "${_vtk_encode_string_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _vtk_encode_string_INPUT)
    message(FATAL_ERROR
      "Missing `INPUT` for vtk_encode_string.")
  endif ()

  if (NOT DEFINED _vtk_encode_string_NAME)
    get_filename_component(_vtk_encode_string_NAME
      "${_vtk_encode_string_INPUT}" NAME_WE)
  endif ()

  if (DEFINED _vtk_encode_string_EXPORT_SYMBOL AND
      NOT DEFINED _vtk_encode_string_EXPORT_HEADER)
    message(FATAL_ERROR
      "Missing `EXPORT_HEADER` when using `EXPORT_SYMBOL`.")
  endif ()

  if (DEFINED _vtk_encode_string_EXPORT_HEADER AND
      NOT DEFINED _vtk_encode_string_EXPORT_SYMBOL)
    message(WARNING
      "Missing `EXPORT_SYMBOL` when using `EXPORT_HEADER`.")
  endif ()

  set(_vtk_encode_string_header
    "${CMAKE_CURRENT_BINARY_DIR}/${_vtk_encode_string_NAME}.h")
  set(_vtk_encode_string_source
    "${CMAKE_CURRENT_BINARY_DIR}/${_vtk_encode_string_NAME}.cxx")

  if (IS_ABSOLUTE "${_vtk_encode_string_INPUT}")
    set(_vtk_encode_string_input
      "${_vtk_encode_string_INPUT}")
  else ()
    set(_vtk_encode_string_input
      "${CMAKE_CURRENT_SOURCE_DIR}/${_vtk_encode_string_INPUT}")
  endif ()

  add_custom_command(
    OUTPUT  ${_vtk_encode_string_header}
            ${_vtk_encode_string_source}
    DEPENDS "${_vtkEncodeString_script_file}"
            "${_vtk_encode_string_input}"
    COMMAND "${CMAKE_COMMAND}"
            "-Dsource_dir=${CMAKE_CURRENT_SOURCE_DIR}"
            "-Dbinary_dir=${CMAKE_CURRENT_BINARY_DIR}"
            "-Dsource_file=${_vtk_encode_string_input}"
            "-Doutput_name=${_vtk_encode_string_NAME}"
            "-Dexport_symbol=${_vtk_encode_string_EXPORT_SYMBOL}"
            "-Dexport_header=${_vtk_encode_string_EXPORT_HEADER}"
            "-D_vtk_encode_string_run=ON"
            -P "${_vtkEncodeString_script_file}")

  if (DEFINED _vtk_encode_string_SOURCE_OUTPUT)
    set("${_vtk_encode_string_SOURCE_OUTPUT}"
      "${_vtk_encode_string_source}"
      PARENT_SCOPE)
  endif ()

  if (DEFINED _vtk_encode_string_HEADER_OUTPUT)
    set("${_vtk_encode_string_HEADER_OUTPUT}"
      "${_vtk_encode_string_header}"
      PARENT_SCOPE)
  endif ()
endfunction ()

if (_vtk_encode_string_run)
  set(output_header "${binary_dir}/${output_name}.h")
  set(output_source "${binary_dir}/${output_name}.cxx")

  file(WRITE "${output_header}" "")
  file(WRITE "${output_source}" "")

  file(APPEND "${output_header}"
    "#ifndef ${output_name}_h\n#define ${output_name}_h\n\n")
  if (export_symbol)
    file(APPEND "${output_header}"
      "#include \"${export_header}\"\n\n${export_symbol} ")
  endif ()
  file(APPEND "${output_header}"
    "extern const char *${output_name};\n\n#endif\n")

  if (IS_ABSOLUTE "${source_file}")
    set(source_file_full "${source_file}")
  else ()
    set(source_file_full "${source_dir}/${source_file}")
  endif ()
  file(READ "${source_file_full}" original_content)

  # Escape literal backslashes.
  string(REPLACE "\\" "\\\\" escaped_content "${original_content}")
  # Escape literal double quotes.
  string(REPLACE "\"" "\\\"" escaped_content "${escaped_content}")
  # Turn newlines into newlines in the C string.
  string(REPLACE "\n" "\\n\"\n\"" escaped_content "${escaped_content}")

  file(APPEND "${output_source}"
    "#include \"${output_name}.h\"\n\nconst char *${output_name} =\n")
  file(APPEND "${output_source}"
    "\"${escaped_content}\";\n")
endif ()
