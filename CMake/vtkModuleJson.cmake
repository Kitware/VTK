macro (_vtk_json_bool output name cond)
  if (${cond})
    set(val "true")
  else ()
    set(val "false")
  endif ()
  string(APPEND "${output}" "\"${name}\": ${val}, ")
  unset(val)
endmacro ()

macro (_vtk_json_string_list output name var)
  set(list "[")
  foreach (value IN LISTS "${var}")
    string(APPEND list "\"${value}\", ")
  endforeach ()
  string(APPEND list "]")
  string(REPLACE ", ]" "]" list "${list}")
  string(APPEND "${output}" "\"${name}\": ${list}, ")
  unset(value)
  unset(list)
endmacro ()

#[==[.md
## JSON output

Information about the modules built and/or available may be dumped to a JSON
file.

```
vtk_module_json(
  MODULES   <module>...
  OUTPUT    <path>)
```

  * `MODULES`: (Required) The modules to output information for.
  * `OUTPUT`: (Required) A JSON file describing the modules built will
    be output to this path. Relative paths are rooted to `CMAKE_BINARY_DIR`.

Example output:

```json
{
  "modules": [
    {
      "name": "...",
      "library_name": "...",
      "enabled": <bool>,
      "implementable": <bool>,
      "third_party": <bool>,
      "wrap_exclude": <bool>,
      "kit": "...",
      "groups": [
        "..."
      ],
      "depends": [
        "..."
      ],
      "optional_depends": [
        "..."
      ],
      "private_depends": [
        "..."
      ],
      "implements": [
        "..."
      ]
    }
  ]
  "kits": [
    {
      "name": "...",
      "enabled": <bool>,
      "modules": [
      ]
    }
  ]
  "groups": [
    "..."
  ]
}
```
#]==]
function (vtk_module_json)
  cmake_parse_arguments(_vtk_json
    ""
    "OUTPUT"
    "MODULES"
    ${ARGN})

  if (_vtk_json_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_json: "
      "${_vtk_json_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _vtk_json_OUTPUT)
    message(FATAL_ERROR
      "The `OUTPUT` argument is required.")
  endif ()

  if (NOT _vtk_json_MODULES)
    message(FATAL_ERROR "No modules given to output.")
  endif ()

  if (NOT IS_ABSOLUTE "${_vtk_json_OUTPUT}")
    set(_vtk_json_OUTPUT "${CMAKE_BINARY_DIR}/${_vtk_json_OUTPUT}")
  endif ()

  set(_vtk_json_kits)

  set(_vtk_json_contents "{")
  string(APPEND _vtk_json_contents "\"modules\": {")
  foreach (_vtk_json_module IN LISTS _vtk_json_MODULES)
    get_property(_vtk_json_description GLOBAL
      PROPERTY "_vtk_module_${_vtk_json_module}_description")
    get_property(_vtk_json_implementable GLOBAL
      PROPERTY "_vtk_module_${_vtk_json_module}_implementable")
    get_property(_vtk_json_third_party GLOBAL
      PROPERTY "_vtk_module_${_vtk_json_module}_third_party")
    get_property(_vtk_json_wrap_exclude GLOBAL
      PROPERTY "_vtk_module_${_vtk_json_module}_wrap_exclude")
    get_property(_vtk_json_kit GLOBAL
      PROPERTY "_vtk_module_${_vtk_json_module}_kit")
    get_property(_vtk_json_depends GLOBAL
      PROPERTY "_vtk_module_${_vtk_json_module}_depends")
    get_property(_vtk_json_private_depends GLOBAL
      PROPERTY "_vtk_module_${_vtk_json_module}_private_depends")
    get_property(_vtk_json_optional_depends GLOBAL
      PROPERTY "_vtk_module_${_vtk_json_module}_optional_depends")
    get_property(_vtk_json_implements GLOBAL
      PROPERTY "_vtk_module_${_vtk_json_module}_implements")

    set(_vtk_json_kit_name "null")
    if (_vtk_json_kit)
      list(APPEND _vtk_json_kits
        "${_vtk_json_kit}")
      set(_vtk_json_kit_name "\"${_vtk_json_kit}\"")
    endif ()
    set(_vtk_json_library_name "null")
    if (TARGET "${_vtk_json_module}")
      get_property(_vtk_json_library_type
        TARGET    "${_vtk_json_module}"
        PROPERTY  TYPE)
      if (NOT _vtk_json_library_type STREQUAL "INTERFACE_LIBRARY")
        get_property(_vtk_json_library
          TARGET    "${_vtk_json_module}"
          PROPERTY  LIBRARY_OUTPUT_NAME)
        set(_vtk_json_library_name "\"${_vtk_json_library}\"")
      endif ()
    endif ()

    string(APPEND _vtk_json_contents "\"${_vtk_json_module}\": {")
    string(APPEND _vtk_json_contents "\"library_name\": ${_vtk_json_library_name}, ")
    string(APPEND _vtk_json_contents "\"description\": \"${_vtk_json_description}\", ")
    _vtk_json_bool(_vtk_json_contents "enabled" "TARGET;${_vtk_json_module}")
    _vtk_json_bool(_vtk_json_contents "implementable" _vtk_json_implementable)
    _vtk_json_bool(_vtk_json_contents "third_party" _vtk_json_third_party)
    _vtk_json_bool(_vtk_json_contents "wrap_exclude" _vtk_json_wrap_exclude)
    string(APPEND _vtk_json_contents "\"kit\": ${_vtk_json_kit_name}, ")
    # _vtk_json_string_list(_vtk_json_contents "groups" _vtk_json_groups)
    _vtk_json_string_list(_vtk_json_contents "depends" _vtk_json_depends)
    _vtk_json_string_list(_vtk_json_contents "optional_depends" _vtk_json_optional_depends)
    _vtk_json_string_list(_vtk_json_contents "private_depends" _vtk_json_private_depends)
    _vtk_json_string_list(_vtk_json_contents "implements" _vtk_json_implements)
    string(APPEND _vtk_json_contents "}, ")
  endforeach ()
  string(APPEND _vtk_json_contents "}, ")

  string(APPEND _vtk_json_contents "\"kits\": {")
  foreach (_vtk_json_kit IN LISTS _vtk_json_kits)
    set(_vtk_json_library_name "null")
    if (TARGET "${_vtk_json_kit}")
      get_property(_vtk_json_library
        TARGET    "${_vtk_json_kit}"
        PROPERTY  LIBRARY_OUTPUT_NAME)
      set(_vtk_json_library_name "\"${_vtk_json_library}\"")
    endif ()

    string(APPEND _vtk_json_contents "\"${_vtk_json_kit}\": {")
    string(APPEND _vtk_json_contents "\"library_name\": ${_vtk_json_library_name}, ")
    _vtk_json_bool(_vtk_json_contents "enabled" "TARGET;${_vtk_json_kit}")
    string(APPEND _vtk_json_contents "}, ")
  endforeach ()
  string(APPEND _vtk_json_contents "}, ")

  string(APPEND _vtk_json_contents "}")
  string(REPLACE ", ]" "]" _vtk_json_contents "${_vtk_json_contents}")
  string(REPLACE ", }" "}" _vtk_json_contents "${_vtk_json_contents}")
  file(GENERATE
    OUTPUT  "${_vtk_json_OUTPUT}"
    CONTENT "${_vtk_json_contents}")
endfunction ()
