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

set(vtk_fixed_size_numeric_types
  "vtkTypeFloat32"
  "vtkTypeFloat64"
  "vtkTypeInt8"
  "vtkTypeInt16"
  "vtkTypeInt32"
  "vtkTypeInt64"
  "vtkTypeUInt8"
  "vtkTypeUInt16"
  "vtkTypeUInt32"
  "vtkTypeUInt64"
)

include(CheckTypeSize)

#[==[.md
# vtk_get_fixed_size_type_mapping
This function creates a mapping from fixed-size types to their corresponding
platform-specific numeric types. The mapping is determined by testing
the actual type definitions on the current platform.
#]==]
function(vtk_get_fixed_size_type_mapping fixed_type output)
  # First check sizes once
  check_type_size("long" _VTK_SIZEOF_LONG)
  check_type_size("long long" _VTK_SIZEOF_LONG_LONG)

  set(vtkTypeInt8_output "signed char")
  set(vtkTypeUInt8_output "unsigned char")
  set(vtkTypeInt16_output "short")
  set(vtkTypeUInt16_output "unsigned short")
  set(vtkTypeInt32_output "int")
  set(vtkTypeUInt32_output "unsigned int")
  if (_VTK_SIZEOF_LONG_LONG EQUAL 8)
    set(vtkTypeInt64_output "long long")
    set(vtkTypeUInt64_output "unsigned long long")
  elseif (_VTK_SIZEOF_LONG EQUAL 8)
    set(vtkTypeInt64_output "long")
    set(vtkTypeUInt64_output "unsigned long")
  else ()
    message(WARNING "No suitable 64-bit integer types found")
    set(vtkTypeInt64_output "")
    set(vtkTypeUInt64_output "")
  endif ()
  set(vtkTypeFloat32_output "float")
  set(vtkTypeFloat64_output "double")

  # Lookup via "dictionary"
  if (DEFINED ${fixed_type}_output)
    set(${output} "${${fixed_type}_output}" PARENT_SCOPE)
  else ()
    set(${output} "" PARENT_SCOPE)
  endif ()
endfunction()

#[==[.md
# vtk_type_to_camel_case
This is a function to generate the CamelCase version of the c++ type name.

vtkIdType is converted to IdType, losing the "vtk" prefix.
#]==]
function(vtk_type_to_camel_case type output)
  if (type STREQUAL "vtkIdType")
    set("${output}" "IdType" PARENT_SCOPE)
    return()
  endif ()

  string(REPLACE " " ";" type_list "${type}")

  set(_cased_type)
  foreach (_word IN LISTS type_list)
    string(SUBSTRING ${_word} 0 1 _first_letter)
    string(TOUPPER ${_first_letter} _first_letter_upper)
    string(REGEX REPLACE "^${_first_letter}" ${_first_letter_upper} _new_word ${_word})
    string(CONCAT _cased_type "${_cased_type}" "${_new_word}")
  endforeach ()

  set("${output}" "${_cased_type}" PARENT_SCOPE)
endfunction()

function(vtk_fixed_size_type_to_without_prefix type prefix output)
  # Remove the prefix from the input type
  string(REPLACE "${prefix}" "" _result "${type}")
  # Export the result to the parent scope
  set(${output} "${_result}" PARENT_SCOPE)
endfunction()
