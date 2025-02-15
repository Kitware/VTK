#[==[.md
# vtkTypeLists

This module provides lists of (c++) data type supported by VTK.
A CamelCase formatted list is also provided to easily construct
class names.

This is useful to generate template instantiation at configure time.
#]==]

# The default set of scalar types:
set(vtk_numeric_types
  "char"
  "double"
  "float"
  "int"
  "long"
  "long long"
  "short"
  "signed char"
  "unsigned char"
  "unsigned int"
  "unsigned long"
  "unsigned long long"
  "unsigned short"
  "vtkIdType"
)

#[==[.md
# vtk_type_to_camel_case
This is a function to generate the CamelCase version of the c++ type name.

vtkIdType is converted to IdType, losing the "vtk" prefix.
#]==]
function(vtk_type_to_camel_case type output)
  if (type STREQUAL "vtkIdType")
    set("${output}" "IdType" PARENT_SCOPE )
    return()
  endif ()

  string(REPLACE " " ";" type_list "${type}")

  set (_cased_type)
  foreach (_word IN LISTS type_list)
    string(SUBSTRING ${_word} 0 1 _first_letter)
    string(TOUPPER ${_first_letter} _first_letter_upper)
    string(REGEX REPLACE "^${_first_letter}" ${_first_letter_upper} _new_word ${_word})
    string(CONCAT _cased_type "${_cased_type}" "${_new_word}")
  endforeach()

  set("${output}" "${_cased_type}" PARENT_SCOPE )
endfunction()
