# This file generates vtkImplicitArrays subclass for concrete type,
# like `vtkConstanteDoubleArray` or `vtkIdTypeCompositeArray`.
#
# Generated classes are not templated thus they can be wrapped.

# Configure `.in` class files depending on the requested backend
# and the concrete c++ type.
macro(_generate_implicit_array_specialization backend vtk_type concrete_type)
  # used inside .in files
  set(VTK_TYPE_NAME "${vtk_type}")
  set(CONCRETE_TYPE "${concrete_type}")

  set(_className "vtk${backend}${VTK_TYPE_NAME}Array")

  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/vtk${backend}TypedArray.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${_className}.h"
    @ONLY)

  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/vtk${backend}TypedArray.cxx.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${_className}.cxx"
    @ONLY)

  # append generated files to current module headers and sources
  list(APPEND headers
    "${CMAKE_CURRENT_BINARY_DIR}/${_className}.h")
  list(APPEND sources
    "${CMAKE_CURRENT_BINARY_DIR}/${_className}.cxx")

  unset(VTK_TYPE_NAME)
  unset(CONCRETE_TYPE)
  unset(_className)
endmacro()

include(vtkTypeLists)

foreach(backend IN ITEMS Constant Affine Indexed Composite)
  foreach (type IN LISTS vtk_numeric_types)
    vtk_type_to_camel_case("${type}" cased_type)
    _generate_implicit_array_specialization("${backend}" "${cased_type}" "${type}")
  endforeach()
endforeach()
